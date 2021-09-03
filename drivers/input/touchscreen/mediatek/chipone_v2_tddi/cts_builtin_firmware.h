static u8 icnl9911c_driver_builtin_firmware[] = {
    // TODO:
};

const static struct cts_firmware cts_driver_builtin_firmwares[] = {
    {
        .name = "Longcheer-Linda",      /* MUST set non-NULL */
        .hwid = CTS_DEV_HWID_ICNL9911C,
        .fwid = CTS_DEV_FWID_ICNL9911C,
        .data = icnl9911c_driver_builtin_firmware,
        .size = ARRAY_SIZE(icnl9911c_driver_builtin_firmware),
    },
};
