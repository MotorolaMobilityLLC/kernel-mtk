#ifndef __UT_TUI_MAIN_H_
#define __UT_TUI_MAIN_H_

#define UT_TUI_CLIENT_DEV "utr_tui"
#define UT_TUI_CLIENT_IOC_MAGIC 0x775A777F

struct ut_tui_data_u32
{
    unsigned int datalen;
    unsigned int data;
};
struct ut_tui_data_u64
{
    unsigned int datalen;
    unsigned long data;
};

#define UT_TUI_DISPLAY_COMMAND_U64 \
    _IOWR(UT_TUI_CLIENT_IOC_MAGIC, 0x01,struct ut_tui_data_u64)
#define UT_TUI_NOTICE_COMMAND_U64 \
    _IOWR(UT_TUI_CLIENT_IOC_MAGIC, 0x02,struct ut_tui_data_u64)


#define UT_TUI_DISPLAY_COMMAND_U32 \
    _IOWR(UT_TUI_CLIENT_IOC_MAGIC, 0x01,struct ut_tui_data_u32)
#define UT_TUI_NOTICE_COMMAND_U32 \
    _IOWR(UT_TUI_CLIENT_IOC_MAGIC, 0x02,struct ut_tui_data_u32)

extern unsigned long tui_notice_message_buff;
extern unsigned long tui_display_message_buff;

#endif
