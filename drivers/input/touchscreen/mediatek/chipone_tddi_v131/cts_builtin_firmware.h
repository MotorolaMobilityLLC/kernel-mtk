static u8 icnl9911_driver_builtin_firmware[] = {
#include"firmware/hlt/Moto_Malta_ChinoE_M6210L_Holitech_MDT6.517_ICNL9911C_V0203_20200911.h"
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