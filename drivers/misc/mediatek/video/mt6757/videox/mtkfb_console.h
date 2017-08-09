/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MTK_FB_CONSOLE_H__
#define __MTK_FB_CONSOLE_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char   UINT8;
typedef unsigned int	UINT32;
typedef int		INT32;
typedef unsigned char   BYTE;

#define MFC_CHECK_RET(expr)             \
do {                                \
	MFC_STATUS ret = (expr);        \
	BUG_ON(!(MFC_STATUS_OK == ret));   \
} while (0)


	typedef enum {
		MFC_STATUS_OK = 0,

		MFC_STATUS_INVALID_ARGUMENT = -1,
		MFC_STATUS_NOT_IMPLEMENTED = -2,
		MFC_STATUS_OUT_OF_MEMORY = -3,
		MFC_STATUS_LOCK_FAIL = -4,
		MFC_STATUS_FATAL_ERROR = -5,
	} MFC_STATUS;


	typedef void *MFC_HANDLE;

/* --------------------------------------------------------------------------- */

	typedef struct {
		struct semaphore sem;

		UINT8 *fb_addr;
		UINT32 fb_width;
		UINT32 fb_height;
		UINT32 fb_bpp;
		UINT32 fg_color;
		UINT32 bg_color;
		UINT32 screen_color;
		UINT32 rows;
		UINT32 cols;
		UINT32 cursor_row;
		UINT32 cursor_col;
		UINT32 font_width;
		UINT32 font_height;
	} MFC_CONTEXT;

/* MTK Framebuffer Console API */

	MFC_STATUS MFC_Open(MFC_HANDLE *handle,
			    void *fb_addr,
			    unsigned int fb_width,
			    unsigned int fb_height,
			    unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color);

	MFC_STATUS MFC_Open_Ex(MFC_HANDLE *handle,
			       void *fb_addr,
			       unsigned int fb_width,
			       unsigned int fb_height,
			       unsigned int fb_pitch,
			       unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color);


	MFC_STATUS MFC_Close(MFC_HANDLE handle);

	MFC_STATUS MFC_SetColor(MFC_HANDLE handle, unsigned int fg_color, unsigned int bg_color);

	MFC_STATUS MFC_ResetCursor(MFC_HANDLE handle);

	MFC_STATUS MFC_Print(MFC_HANDLE handle, const char *str);

	MFC_STATUS MFC_LowMemory_Printf(MFC_HANDLE handle, const char *str, UINT32 fg_color,
					UINT32 bg_color);

	MFC_STATUS MFC_SetMem(MFC_HANDLE handle, const char *str, UINT32 color);
	UINT32 MFC_Get_Cursor_Offset(MFC_HANDLE handle);

	/* screen logger */
	typedef struct _screen_logger {
		struct list_head list;
		char *obj;
		char *message;
	} screen_logger;

	typedef enum _message_mode {
		MESSAGE_REPLACE = 0,
		MESSAGE_APPEND = 1
	} message_mode;

	void screen_logger_init(void);
	void screen_logger_add_message(char *obj, message_mode mode, char *message);
	void screen_logger_remove_message(char *obj);
	void screen_logger_print(MFC_HANDLE handle);
	void screen_logger_empty(void);
	void screen_logger_test_case(MFC_HANDLE handle);
#ifdef __cplusplus
}
#endif
#endif				/* __MTK_FB_CONSOLE_H__ */
