#include <nds.h>
#include <stdio.h>
#include <stdarg.h>

#include "platform.h"
#include "console.h"
#include "ncgcpp/ntrcard.h"
#include "delay.h"
#include "nds_platform.h"
#include "binaries.h"


using namespace std;
using namespace flashcart_core;
ncgc::NTRCard *m_card;

/*
void _sendCommand(const uint8_t *cmdbuf, uint16_t response_len, uint8_t *resp, uint32_t flags) {
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
    cardPolledTransfer(defaultFlags | CARD_ACTIVATE | CARD_nRESET,
                       (u32*)resp, (response_len / 4), reversed);
}

bool platform::sendCommand(const uint8_t *cmdbuf, uint16_t response_len, uint8_t *resp, ntrcard::OpFlags flags) {
    _sendCommand(cmdbuf, response_len, resp, flags);
    return true;
}
*/
void platform::showProgress(uint32_t curr, uint32_t total, const char *status_string) {
    printProgress(status_string, curr, total);
}

/*void platform::ioDelay(unsigned long delay) {
    swiDelay(delay);
}*/

/*int32_t platform::resetCard() {
    reset();
    return 0;
}*/
auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048] {
    switch (key) {
        default:
        case BlowfishKey::NTR:
            return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_ntr_bin));
        case BlowfishKey::B9Retail:
            return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_retail_bin));
        case BlowfishKey::B9Dev:
            return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_dev_bin));
    }
    }
/*void platform::initBlowfishPS(uint32_t (&ps)[ntrcard::BLOWFISH_PS_N], ntrcard::BlowfishKey key) {
    const void *ptr;
    switch (key) {
        default: // blah
        case ntrcard::BlowfishKey::NTR:
            if (blowfish_ntr_bin_size != sizeof(ps)) {
                return;
            }
            ptr = blowfish_ntr_bin;
            break;
        case ntrcard::BlowfishKey::B9RETAIL:
            if (blowfish_retail_bin_size != sizeof(ps)) {
                return;
            }
            ptr = blowfish_retail_bin;
            break;
        case ntrcard::BlowfishKey::B9DEV:
            if (blowfish_dev_bin_size != sizeof(ps)) {
                return;
            }
            ptr = blowfish_dev_bin;
            break;
    }

    memcpy(ps, ptr, sizeof(ps));
}*/

/*void platform::initKey2Seed(std::uint64_t x, std::uint64_t y) {
    REG_ROMCTRL = 0;

    uint32_t xl = (uint32_t)(x & 0xFFFFFFFF);
    uint32_t yl = (uint32_t)(y & 0xFFFFFFFF);
    uint16_t xh = (uint16_t)((x >> 32) & 0x7F);
    uint16_t yh = (uint16_t)((y >> 32) & 0x7F);

    REG_CARD_1B0 = xl;
    REG_CARD_1B4 = yl;
    REG_CARD_1B8 = xh;
    REG_CARD_1BA = yh;

    REG_ROMCTRL = CARD_nRESET| CARD_SEC_SEED | CARD_SEC_EN | CARD_SEC_DAT;
}
*/
#ifdef DEBUG_PRINT
int platform::logMessage(log_priority priority, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    iprintf("\n");
    viprintf(fmt, args);
    va_end(args);
    iprintf("\n");

    waitPressA();
    return 0;
}
#endif

const uint8_t dummyCommand[8] = {0x9F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void reset() {
#ifndef NDSI_MODE
    iprintf("Please eject the cartridge,\n");
    iprintf("remove the SD card,\n");
    iprintf("then reinsert the cartridge.\n\n");

    waitPressA();

    ncgc::c::ncgc_ncard_t& state = m_card->rawState();
#endif
        //m_card->sendCommand(m_card, dummyCommand, 0x2000, NULL, 32)
    //m_card->sendCommand(card, CMD_RAW_DUMMY, 0x2000, NULL, 32);
}
