static u8 icnl9911_driver_builtin_firmware_hjc[] = {
#include"firmware/mdt/Moto-Blackjack-Chipone-ICNL9911S-MDT_Hmode_V0112_20200206.h"
};

static u8 icnl9911_driver_builtin_firmware_truly_601[] = {
#include"firmware/truly_608/Moto-Fiji-Chipone-ICNL9911S-MDT_Long-V_20200512_V0111.h"
};

static u8 icnl9911_driver_builtin_firmware_rs[] = {
#include"firmware/truly/Moto-Blackjack-Chipone-ICNL9911S-TRULY_Vmode_V0106_20200219.h"
};

static u8 icnl9911_driver_builtin_firmware_easyquick_608[] = {
#include"firmware/easyquick_608/Moto-Fiji-Chipone-ICNL9911S-MDT_Long-V_20200512_V0111.h"
};

const static struct cts_firmware cts_driver_builtin_firmwares_truly_601[] = {
    {
        .name = "Ontim-Moto FIJI truly 601",      /* MUST set non-NULL */
        .hwid = CTS_DEV_HWID_ICNL9911S,
        .fwid = CTS_DEV_FWID_ICNL9911S,
        .data = icnl9911_driver_builtin_firmware_truly_601,
        .size = ARRAY_SIZE(icnl9911_driver_builtin_firmware_truly_601),
    },
};

const static struct cts_firmware cts_driver_builtin_firmwares_easyquick_608[] = {
    {
        .name = "Ontim-Moto FIJI easyquick 608",      /* MUST set non-NULL */
        .hwid = CTS_DEV_HWID_ICNL9911S,
        .fwid = CTS_DEV_FWID_ICNL9911S,
        .data = icnl9911_driver_builtin_firmware_easyquick_608,
        .size = ARRAY_SIZE(icnl9911_driver_builtin_firmware_easyquick_608),
    },
};

const static struct cts_firmware cts_driver_builtin_firmwares_hjc[] = {
    {
        .name = "Ontim-Moto Blackjack+ hjc",      /* MUST set non-NULL */
        .hwid = CTS_DEV_HWID_ICNL9911S,
        .fwid = CTS_DEV_FWID_ICNL9911S,
        .data = icnl9911_driver_builtin_firmware_hjc,
        .size = ARRAY_SIZE(icnl9911_driver_builtin_firmware_hjc),
    },
};

const static struct cts_firmware cts_driver_builtin_firmwares_rs[] = {
    {
        .name = "Ontim-Moto Blackjack+ rs",      /* MUST set non-NULL */
        .hwid = CTS_DEV_HWID_ICNL9911S,
        .fwid = CTS_DEV_FWID_ICNL9911S,
        .data = icnl9911_driver_builtin_firmware_rs,
        .size = ARRAY_SIZE(icnl9911_driver_builtin_firmware_rs),
    },
};

