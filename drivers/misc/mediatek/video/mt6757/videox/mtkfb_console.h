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
	enum MFC_STATUS ret = (expr);        \
	WARN_ON(!(ret == MFC_STATUS_OK));   \
} while (0)


	enum MFC_STATUS {
		MFC_STATUS_OK = 0,

		MFC_STATUS_INVALID_ARGUMENT = -1,
		MFC_STATUS_NOT_IMPLEMENTED = -2,
		MFC_STATUS_OUT_OF_MEMORY = -3,
		MFC_STATUS_LOCK_FAIL = -4,
		MFC_STATUS_FATAL_ERROR = -5,
	};

	typedef void *MFC_HANDLE;

/* --------------------------------------------------------------------------- */

	struct MFC_CONTEXT {
		struct semaphore sem;

		unsigned char *fb_addr;
		unsigned int fb_width;
		unsigned int fb_height;
		unsigned int fb_bpp;
		unsigned int fg_color;
		unsigned int bg_color;
		unsigned int screen_color;
		unsigned int rows;
		unsigned int cols;
		unsigned int cursor_row;
		unsigned int cursor_col;
		unsigned int font_width;
		unsigned int font_height;
	};

/* MTK Framebuffer Console API */

	enum MFC_STATUS MFC_Open(void **handle,
			    void *fb_addr,
			    unsigned int fb_width,
			    unsigned int fb_height,
			    unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color);

	enum MFC_STATUS MFC_Open_Ex(void **handle,
			       void *fb_addr,
			       unsigned int fb_width,
			       unsigned int fb_height,
			       unsigned int fb_pitch,
			       unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color);


	enum MFC_STATUS MFC_Close(void *handle);

	enum MFC_STATUS MFC_SetColor(void *handle, unsigned int fg_color, unsigned int bg_color);

	enum MFC_STATUS MFC_ResetCursor(void *handle);

	enum MFC_STATUS MFC_Print(void *handle, const char *str);

	enum MFC_STATUS MFC_LowMemory_Printf(void *handle, const char *str, unsigned int fg_color,
					unsigned int bg_color);

	enum MFC_STATUS MFC_SetMem(void *handle, const char *str, unsigned int color);
	unsigned int MFC_Get_Cursor_Offset(void *handle);

	/* screen logger */
	struct screen_logger {
		struct list_head list;
		char *obj;
		char *message;
	};

	enum message_mode {
		MESSAGE_REPLACE = 0,
		MESSAGE_APPEND = 1
	};

	void screen_logger_init(void);
	void screen_logger_add_message(char *obj, enum message_mode mode, char *message);
	void screen_logger_remove_message(char *obj);
	void screen_logger_print(void *handle);
	void screen_logger_empty(void);
	void screen_logger_test_case(void *handle);
#ifdef __cplusplus
}
#endif
#endif				/* __MTK_FB_CONSOLE_H__ */
