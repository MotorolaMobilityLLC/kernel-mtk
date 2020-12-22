static u8 icnl9911_driver_builtin_firmware[] = {
#include"firmware/hlt/ICNL9911C_HLT_MOTO_MALTA_20201221_V020D.h"
};
const static struct cts_firmware cts_driver_builtin_firmwares[] = {
    {
        .name = "OEM-Project",      /* MUST set non-NULL */
        .hwid = CTS_DEV_HWID_ICNL9911C,
        .fwid = CTS_DEV_FWID_ICNL9911C,
        .data = icnl9911_driver_builtin_firmware,
        .size = ARRAY_SIZE(icnl9911_driver_builtin_firmware),
    },
};