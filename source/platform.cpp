#include <nds.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>

#include "ntrcard.h"
#include "device.h"
#include "console.h"
#include "platform.h"
#include "delay.h"
using flashcart_core::Flashcart;
namespace ntrcard = flashcart_core::ntrcard;

#define ROMCNT_ACTIVATE         (1u << 31)              // begin operation (CS low when set)
#define ROMCNT_BUSY             (ROMCNT_ACTIVATE)       // operation in progress i.e. CS still low
#define ROMCNT_WR               (1u << 30)              // card write enable
#define ROMCNT_NRESET           (1u << 29)              // RESET high when set
#define ROMCNT_SEC_LARGE        (1u << 28)              // Use "other" secure area mode, which tranfers blocks of 0x1000 bytes at a time
#define ROMCNT_CLK_SLOW         (1u << 27)              // Transfer clock rate (0 = 6.7MHz, 1 = 4.2MHz)
#define ROMCNT_BLK_SIZE(n)      (((n) & 0x7u) << 24)    // Transfer block size, (0 = None, 1..6 = (0x100 << n) bytes, 7 = 4 bytes)
#define ROMCNT_BLK_SIZE_MASK    (ROMCNT_BLK_SIZE(7))
#define ROMCNT_DATA_READY       (1u << 23)              // REG_FIFO is ready to be read
#define ROMCNT_SEC_CMD          (1u << 22)              // The command transfer will be hardware encrypted (KEY2)
#define ROMCNT_DELAY2(n)        (((n) & 0x3Fu) << 16)   // Transfer delay length part 2
#define ROMCNT_DELAY2_MASK      (ROMCNT_DELAY2(0x3F))
#define ROMCNT_SEC_SEED         (1u << 15)              // Apply encryption (KEY2) seed to hardware registers
#define ROMCNT_SEC_EN           (1u << 14)              // Security enable
#define ROMCNT_SEC_DAT          (1u << 13)              // The data transfer will be hardware encrypted (KEY2)
#define ROMCNT_DELAY1(n)        ((n) & 0x1FFFu)         // Transfer delay length part 1
#define ROMCNT_DELAY1_MASK      (ROMCNT_DELAY1(0x1FFF))

#define REG_CMD                 (*reinterpret_cast<volatile uint64_t *>(0x10164008))
#define REG_FIFO                (*reinterpret_cast<volatile uint32_t *>(0x1016401C))
#define REG_ROMCNT              (*reinterpret_cast<volatile uint32_t *>(0x10164004))


namespace flashcart_core {
namespace platform {
void sendCommandold(const uint8_t *cmdbuf, uint16_t response_len, uint8_t *resp, uint32_t flags) {
    u8 reversed[8];
    for (int i = 0; i < 8; i++) {
        reversed[7 - i] = cmdbuf[i];
    }

    u32 defaultFlags = flags;
    switch (response_len & 0xfffffffc) {
        case 0:
            defaultFlags |= 0;
            break;
        case 4:
            defaultFlags |= CARD_BLK_SIZE(7);
            break;
        case 512:
            defaultFlags |= CARD_BLK_SIZE(1);
            break;
        case 8192:
            defaultFlags |= CARD_BLK_SIZE(5);
            break;
        case 16384:
            defaultFlags |= CARD_BLK_SIZE(6);
            break;
        default:
            defaultFlags |= CARD_BLK_SIZE(4);
            break;
    }

#ifndef NDSI_MODE
    // NDSL only
    cardPolledTransfer(defaultFlags | CARD_ACTIVATE | CARD_nRESET,
                       (u32*)resp, response_len, reversed);
#else
    // NDSL, DSLi, etc...
    cardPolledTransfer(defaultFlags | CARD_ACTIVATE | CARD_nRESET |
                       CARD_SEC_CMD | CARD_SEC_EN | CARD_SEC_DAT,
                       (u32*)resp, response_len, reversed);
#endif
}

bool sendCommand(const uint8_t *cmdbuf, uint16_t response_len, uint8_t *const resp, ntrcard::OpFlags flags) {
    uint32_t blksizeflag;
    switch (response_len) {
        default: return false;
        case 0: blksizeflag = 0; break;
        case 4: blksizeflag = 7; break;
        case 0x200: blksizeflag = 1; break;
        case 0x400: blksizeflag = 2; break;
        case 0x800: blksizeflag = 3; break;
        case 0x1000: blksizeflag = 4; break;
        case 0x2000: blksizeflag = 5; break;
        case 0x4000: blksizeflag = 6; break;
    }
    REG_CMD = *reinterpret_cast<const uint64_t *>(cmdbuf);
    uint32_t bitflags = ROMCNT_ACTIVATE | ROMCNT_NRESET | ROMCNT_BLK_SIZE(blksizeflag) |
        ROMCNT_DELAY1(flags.pre_delay) | ROMCNT_DELAY2(flags.post_delay) |
        (flags.large_secure_area_read ? ROMCNT_SEC_LARGE : 0) |
        (flags.key2_command ? ROMCNT_SEC_CMD : 0) |
        (flags.key2_response ? ROMCNT_SEC_DAT : 0) |
        (flags.slow_clock ? ROMCNT_CLK_SLOW : 0) |
        ((flags.key2_command || flags.key2_response) ? ROMCNT_SEC_EN : 0);
    platform::logMessage(LOG_DEBUG, "ROMCNT = 0x%08X", bitflags);
    REG_ROMCNT = bitflags;

    uint32_t *cur = reinterpret_cast<uint32_t *>(resp);
    uint32_t ctr = 0;
    do {
        if (REG_ROMCNT & ROMCNT_DATA_READY) {
            uint32_t data = REG_FIFO;
            if (resp && ctr < response_len) {
                *(cur++) = data;
                ctr += 4;
            } else {
                (void)data;
            }
        }
	} while (REG_ROMCNT & ROMCNT_BUSY);
    return true;
}
void ioDelay(std::uint32_t us) {
    ::ioDelay(us);
}
const uint8_t dummyCommand[8] = {0x9F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
int32_t resetCard() {
    uint8_t *garbage = (uint8_t*)malloc(sizeof(uint8_t) * 0x2000);
    if (!garbage) {
        // FIXME
        return 1;
    }
    flashcart_core::platform::sendCommandold(dummyCommand, 0x2000, garbage, 32);
    free(garbage);
    return 0;
}
void initBlowfishPS(std::uint32_t (&ps)[ntrcard::BLOWFISH_PS_N]) {
    std::memcpy(ps, reinterpret_cast<void *>(0x01FFE428), sizeof(ps));
    static_assert(sizeof(ps) == 0x1048, "Wrong Blowfish PS size");
}
}
}


const uint8_t chipIDCommand[8] = {0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

uint32_t getChipID() {
    uint32_t chipID;
    flashcart_core::platform::resetCard();
    flashcart_core::platform::sendCommandold(chipIDCommand, 4, (uint8_t*)&chipID, 32);
    return chipID;
}
