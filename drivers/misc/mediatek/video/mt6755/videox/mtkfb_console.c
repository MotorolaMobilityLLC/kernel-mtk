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

#include <linux/font.h>
#include <linux/string.h>
#include <linux/semaphore.h>
#include <linux/slab.h>

#include <linux/bug.h>
#include <linux/types.h>
#include "mtkfb_console.h"
#include "ddp_hal.h"

/* --------------------------------------------------------------------------- */

#define MFC_WIDTH           (ctxt->fb_width)
#define MFC_HEIGHT          (ctxt->fb_height)
#define MFC_BPP             (ctxt->fb_bpp)
#define MFC_PITCH           (ctxt->fb_pitch)
/* #define MFC_PITCH           (MFC_WIDTH * MFC_BPP) */

#define MFC_FG_COLOR        (ctxt->fg_color)
#define MFC_BG_COLOR        (ctxt->bg_color)

#define MFC_FONT            font_vga_8x16
#define MFC_FONT_WIDTH      (MFC_FONT.width)
#define MFC_FONT_HEIGHT     (MFC_FONT.height)
#define MFC_FONT_DATA       (MFC_FONT.data)

#define MFC_ROW_SIZE        (MFC_FONT_HEIGHT * MFC_PITCH)
#define MFC_ROW_FIRST       ((uint8_t *)(ctxt->fb_addr))
#define MFC_ROW_SECOND      (MFC_ROW_FIRST + MFC_ROW_SIZE)
#define MFC_ROW_LAST        (MFC_ROW_FIRST + MFC_SIZE - MFC_ROW_SIZE)
#define MFC_SIZE            (MFC_ROW_SIZE * ctxt->rows)
#define MFC_SCROLL_SIZE     (MFC_SIZE - MFC_ROW_SIZE)

#define MAKE_TWO_RGB565_COLOR(high, low)  (((low) << 16) | (high))

inline MFC_STATUS MFC_LOCK(MFC_CONTEXT *ctxt)
{
	if (down_interruptible(&ctxt->sem)) {
		pr_err("[MFC] ERROR: Can't get semaphore in %s()\n",
			__func__);
		BUG_ON(1);
		return MFC_STATUS_LOCK_FAIL;
	}
	return MFC_STATUS_OK;
}

inline void MFC_UNLOCK(MFC_CONTEXT *ctxt)
{
	up(&ctxt->sem);
}

/* --------------------------------------------------------------------------- */
uint32_t MFC_Get_Cursor_Offset(MFC_HANDLE handle)
{
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) handle;

	uint32_t offset =
	    ctxt->cursor_col * MFC_FONT_WIDTH * MFC_BPP +
	    ctxt->cursor_row * MFC_FONT_HEIGHT * MFC_PITCH;

	return offset;
}

static void _MFC_DrawChar(MFC_CONTEXT *ctxt, uint32_t x, uint32_t y, char c)
{
	uint8_t ch = *((uint8_t *)&c);
	const uint8_t *cdat;
	uint8_t *dest;
	int32_t rows, cols, offset;

	int font_draw_table16[4];

	BUG_ON(!(x <= (MFC_WIDTH - MFC_FONT_WIDTH)));
	BUG_ON(!(y <= (MFC_HEIGHT - MFC_FONT_HEIGHT)));

	offset = y * MFC_PITCH + x * MFC_BPP;
	dest = (MFC_ROW_FIRST + offset);

	switch (MFC_BPP) {
	/* RGB 565 */
	case 2:
		font_draw_table16[0] = MAKE_TWO_RGB565_COLOR(MFC_BG_COLOR, MFC_BG_COLOR);
		font_draw_table16[1] = MAKE_TWO_RGB565_COLOR(MFC_BG_COLOR, MFC_FG_COLOR);
		font_draw_table16[2] = MAKE_TWO_RGB565_COLOR(MFC_FG_COLOR, MFC_BG_COLOR);
		font_draw_table16[3] = MAKE_TWO_RGB565_COLOR(MFC_FG_COLOR, MFC_FG_COLOR);

		cdat = (const uint8_t *)MFC_FONT_DATA + ch * MFC_FONT_HEIGHT;

		for (rows = MFC_FONT_HEIGHT; rows--; dest += MFC_PITCH) {
			uint8_t bits = *cdat++;

			((uint32_t *) dest)[0] = font_draw_table16[bits >> 6];
			((uint32_t *) dest)[1] = font_draw_table16[bits >> 4 & 3];
			((uint32_t *) dest)[2] = font_draw_table16[bits >> 2 & 3];
			((uint32_t *) dest)[3] = font_draw_table16[bits & 3];
		}
		break;
	/* RGB 888 */
	case 3:
		cdat = (const uint8_t *)MFC_FONT_DATA + ch * MFC_FONT_HEIGHT;
		for (rows = MFC_FONT_HEIGHT; rows--; dest += MFC_PITCH) {
			uint8_t bits = *cdat++;
			uint8_t *tmp = dest;

			for (cols = 0; cols < 8; ++cols) {
				uint32_t color = ((bits >> (7 - cols)) & 0x1) ? MFC_FG_COLOR : MFC_BG_COLOR;
				((uint8_t *)tmp)[0] = color & 0xff;
				((uint8_t *)tmp)[1] = (color >> 8) & 0xff;
				((uint8_t *)tmp)[2] = (color >> 16) & 0xff;
				tmp += 3;
			}
		}
		break;
	/* ARGB 8888 */
	case 4:
		cdat = (const uint8_t *)MFC_FONT_DATA + ch * MFC_FONT_HEIGHT;
		for (rows = MFC_FONT_HEIGHT; rows--; dest += MFC_PITCH) {
			uint8_t bits = *cdat++;
			uint8_t *tmp = dest;

			for (cols = 0; cols < 8; ++cols) {
				uint32_t color = ((bits >> (7 - cols)) & 0x1) ? MFC_FG_COLOR : MFC_BG_COLOR;
				((uint8_t *)tmp)[1] = color & 0xff;
				((uint8_t *)tmp)[2] = (color >> 8) & 0xff;
				((uint8_t *)tmp)[3] = (color >> 16) & 0xff;
				((uint8_t *)tmp)[0] = (color >> 16) & 0xff;
				tmp += 4;
			}
		}
		break;
	default:
		BUG_ON(1);
	}
}


static void _MFC_ScrollUp(MFC_CONTEXT *ctxt)
{
	const uint32_t BG_COLOR = MAKE_TWO_RGB565_COLOR(MFC_BG_COLOR, MFC_BG_COLOR);

	uint32_t *ptr = (uint32_t *) MFC_ROW_LAST;
	int i = MFC_ROW_SIZE / sizeof(uint32_t);

	memcpy(MFC_ROW_FIRST, MFC_ROW_SECOND, MFC_SCROLL_SIZE);

	while (--i >= 0)
		*ptr++ = BG_COLOR;
}


static void _MFC_Newline(MFC_CONTEXT *ctxt)
{
	/* /Bin:add for filling the color for the blank of this column */
	while (ctxt->cursor_col < ctxt->cols) {
		_MFC_DrawChar(ctxt,
			      ctxt->cursor_col * MFC_FONT_WIDTH,
			      ctxt->cursor_row * MFC_FONT_HEIGHT, ' ');

		++ctxt->cursor_col;
	}
	++ctxt->cursor_row;
	ctxt->cursor_col = 0;

	/* Check if we need to scroll the terminal */
	if (ctxt->cursor_row >= ctxt->rows) {
		/* Scroll everything up */
		_MFC_ScrollUp(ctxt);

		/* Decrement row number */
		--ctxt->cursor_row;
	}
}


#define CHECK_NEWLINE()                     \
do {                                    \
	if (ctxt->cursor_col >= ctxt->cols) \
		_MFC_Newline(ctxt);             \
} while (0)

static void _MFC_Putc(MFC_CONTEXT *ctxt, const char c)
{
	CHECK_NEWLINE();

	switch (c) {
	case '\n':		/* next line */
		_MFC_Newline(ctxt);
		break;

	case '\r':		/* carriage return */
		ctxt->cursor_col = 0;
		break;

	case '\t':		/* tab 8 */
		ctxt->cursor_col += 8;
		ctxt->cursor_col &= ~0x0007;
		CHECK_NEWLINE();
		break;

	default:		/* draw the char */
		_MFC_DrawChar(ctxt,
			      ctxt->cursor_col * MFC_FONT_WIDTH,
			      ctxt->cursor_row * MFC_FONT_HEIGHT, c);
		++ctxt->cursor_col;
		CHECK_NEWLINE();
	}
}

/* --------------------------------------------------------------------------- */

MFC_STATUS MFC_Open(MFC_HANDLE *handle,
		    void *fb_addr,
		    unsigned int fb_width,
		    unsigned int fb_height,
		    unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color)
{
	MFC_CONTEXT *ctxt = NULL;

	if (NULL == handle || NULL == fb_addr)
		return MFC_STATUS_INVALID_ARGUMENT;

	/* if (fb_bpp != 2) */
	/* return MFC_STATUS_NOT_IMPLEMENTED; */

	ctxt = kzalloc(sizeof(MFC_CONTEXT), GFP_KERNEL);
	if (!ctxt)
		return MFC_STATUS_OUT_OF_MEMORY;

/* init_MUTEX(&ctxt->sem); */
	sema_init(&ctxt->sem, 1);
	ctxt->fb_addr = fb_addr;
	ctxt->fb_width = fb_width;
	ctxt->fb_height = fb_height;
	ctxt->fb_bpp = fb_bpp;
	ctxt->fg_color = fg_color;
	ctxt->bg_color = bg_color;
	ctxt->rows = fb_height / MFC_FONT_HEIGHT;
	ctxt->cols = fb_width / MFC_FONT_WIDTH;
	ctxt->font_width = MFC_FONT_WIDTH;
	ctxt->font_height = MFC_FONT_HEIGHT;
	ctxt->fb_pitch = fb_width * fb_bpp;

	*handle = ctxt;

	return MFC_STATUS_OK;
}

MFC_STATUS MFC_Open_Ex(MFC_HANDLE *handle,
		       void *fb_addr,
		       unsigned int fb_width,
		       unsigned int fb_height,
		       unsigned int fb_pitch,
		       unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color)
{

	MFC_CONTEXT *ctxt = NULL;

	if (NULL == handle || NULL == fb_addr)
		return MFC_STATUS_INVALID_ARGUMENT;

	if (fb_bpp != 2)
		return MFC_STATUS_NOT_IMPLEMENTED;	/* only support RGB565 */

	ctxt = kzalloc(sizeof(MFC_CONTEXT), GFP_KERNEL);
	if (!ctxt)
		return MFC_STATUS_OUT_OF_MEMORY;

/* init_MUTEX(&ctxt->sem); */
	sema_init(&ctxt->sem, 1);
	ctxt->fb_addr = fb_addr;
	ctxt->fb_width = fb_pitch;
	ctxt->fb_height = fb_height;
	ctxt->fb_bpp = fb_bpp;
	ctxt->fg_color = fg_color;
	ctxt->bg_color = bg_color;
	ctxt->rows = fb_height / MFC_FONT_HEIGHT;
	ctxt->cols = fb_width / MFC_FONT_WIDTH;
	ctxt->font_width = MFC_FONT_WIDTH;
	ctxt->font_height = MFC_FONT_HEIGHT;
	ctxt->fb_pitch = fb_pitch * fb_bpp;

	*handle = ctxt;

	return MFC_STATUS_OK;

}

MFC_STATUS MFC_Open_Ex_v2(MFC_HANDLE *handle,
			  void *fb_addr,
			  unsigned int fb_width,
			  unsigned int fb_height,
			  unsigned int fb_pitch_in_bytes,
			  unsigned int fb_bpp, unsigned int fg_color, unsigned int bg_color)
{

	MFC_CONTEXT *ctxt = NULL;

	if (NULL == handle || NULL == fb_addr)
		return MFC_STATUS_INVALID_ARGUMENT;

	ctxt = kzalloc(sizeof(MFC_CONTEXT), GFP_KERNEL);
	if (!ctxt)
		return MFC_STATUS_OUT_OF_MEMORY;

/* init_MUTEX(&ctxt->sem); */
	sema_init(&ctxt->sem, 1);
	ctxt->fb_addr = fb_addr;
	ctxt->fb_width = fb_pitch_in_bytes;
	ctxt->fb_height = fb_height;
	ctxt->fb_bpp = fb_bpp;
	ctxt->fg_color = fg_color;
	ctxt->bg_color = bg_color;
	ctxt->rows = fb_height / MFC_FONT_HEIGHT;
	ctxt->cols = fb_width / MFC_FONT_WIDTH;
	ctxt->font_width = MFC_FONT_WIDTH;
	ctxt->font_height = MFC_FONT_HEIGHT;
	ctxt->fb_pitch = fb_pitch_in_bytes;

	*handle = ctxt;

	return MFC_STATUS_OK;
}

MFC_STATUS MFC_Close(MFC_HANDLE handle)
{
	if (!handle)
		return MFC_STATUS_INVALID_ARGUMENT;

	kfree(handle);

	return MFC_STATUS_OK;
}


MFC_STATUS MFC_SetColor(MFC_HANDLE handle, unsigned int fg_color, unsigned int bg_color)
{
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) handle;

	if (!ctxt)
		return MFC_STATUS_INVALID_ARGUMENT;

	MFC_LOCK(ctxt);
	ctxt->fg_color = fg_color;
	ctxt->bg_color = bg_color;
	MFC_UNLOCK(ctxt);

	return MFC_STATUS_OK;
}


MFC_STATUS MFC_ResetCursor(MFC_HANDLE handle)
{
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) handle;

	if (!ctxt)
		return MFC_STATUS_INVALID_ARGUMENT;

	MFC_LOCK(ctxt);
	ctxt->cursor_row = ctxt->cursor_col = 0;
	MFC_UNLOCK(ctxt);

	return MFC_STATUS_OK;
}


MFC_STATUS MFC_Print(MFC_HANDLE handle, const char *str)
{
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) handle;
	int count = 0;

	if (!ctxt || !str)
		return MFC_STATUS_INVALID_ARGUMENT;

	MFC_LOCK(ctxt);

	count = strlen(str);

	while (count--)
		_MFC_Putc(ctxt, *str++);

	MFC_UNLOCK(ctxt);

	return MFC_STATUS_OK;
}

MFC_STATUS MFC_SetMem(MFC_HANDLE handle, const char *str, uint32_t color)
{
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) handle;
	int count = 0;
	int i, j;
	uint32_t *ptr;

	if (!ctxt || !str)
		return MFC_STATUS_INVALID_ARGUMENT;

	MFC_LOCK(ctxt);

	count = strlen(str);
	count = count * MFC_FONT_WIDTH;

	for (j = 0; j < MFC_FONT_HEIGHT; j++) {
		ptr = (uint32_t *) (ctxt->fb_addr + (j + 1) * MFC_PITCH - count * ctxt->fb_bpp);
		for (i = 0; i < count * ctxt->fb_bpp / sizeof(uint32_t); i++)
			*ptr++ = color;
	}

	MFC_UNLOCK(ctxt);

	return MFC_STATUS_OK;
}

MFC_STATUS MFC_LowMemory_Printf(MFC_HANDLE handle, const char *str, uint32_t fg_color,
				uint32_t bg_color)
{
	MFC_CONTEXT *ctxt = (MFC_CONTEXT *) handle;
	int count = 0;
	unsigned int col, row, fg_color_mfc, bg_color_mfc;

	if (!ctxt || !str)
		return MFC_STATUS_INVALID_ARGUMENT;

	MFC_LOCK(ctxt);

	count = strlen(str);
/* //store cursor_col and row for printf low memory char temply */
	row = ctxt->cursor_row;
	col = ctxt->cursor_col;
	ctxt->cursor_row = 0;
	ctxt->cursor_col = ctxt->cols - count;
	fg_color_mfc = ctxt->fg_color;
	bg_color_mfc = ctxt->bg_color;

/* ///////// */
	ctxt->fg_color = fg_color;
	ctxt->bg_color = bg_color;
	while (count--)
		_MFC_Putc(ctxt, *str++);

/* //restore cursor_col and row for printf low memory char temply */
	ctxt->cursor_row = row;
	ctxt->cursor_col = col;
	ctxt->fg_color = fg_color_mfc;
	ctxt->bg_color = bg_color_mfc;
/* ///////// */


	MFC_UNLOCK(ctxt);

	return MFC_STATUS_OK;
}

/* screen logger. */
static screen_logger logger_head;
static int screen_logger_inited;
void screen_logger_init(void)
{
	if (0 == screen_logger_inited) {
		INIT_LIST_HEAD(&logger_head.list);
		screen_logger_inited = 1;
	}
}

void screen_logger_add_message(char *obj, message_mode mode, char *message)
{
	screen_logger *p;
	int add_new = 1;
	char *tmp1, *tmp2;
	unsigned int len = 0;
	screen_logger_init();
	list_for_each_entry(p, &logger_head.list, list) {
		if (strcmp(p->obj, obj) == 0) {
			switch (mode) {
			case MESSAGE_REPLACE:
				tmp1 = p->message;
				tmp2 = kstrdup(message, GFP_KERNEL);
				p->message = tmp2;
				kfree(tmp1);
				break;
			case MESSAGE_APPEND:
				len = strlen(p->message) + strlen(message);
				tmp1 = kmalloc(sizeof(char) * (len + 1), GFP_KERNEL);
				strcpy(tmp1, p->message);
				strcat(tmp1, message);
				tmp2 = p->message;
				p->message = tmp1;
				kfree(tmp2);
				break;
			default:
				break;
			}
			add_new = 0;
		}
	}
	if (1 == add_new) {
		screen_logger *logger = kmalloc(sizeof(screen_logger), GFP_KERNEL);
		logger->obj = kstrdup(obj, GFP_KERNEL);
		logger->message = kstrdup(message, GFP_KERNEL);
		list_add_tail(&logger->list, &logger_head.list);
	}
}

void screen_logger_remove_message(char *obj)
{
	screen_logger *p;
	list_for_each_entry(p, &logger_head.list, list) {
		if (0 == strcmp(p->obj, obj)) {
			kfree(p->obj);
			kfree(p->message);
			/* memory leak. TODO: delete completely. */
			list_del(&p->list);
			kfree(p);
			break;
		}
	}
	p = NULL;
}

void screen_logger_print(MFC_HANDLE handle)
{
	screen_logger *p;
	list_for_each_entry(p, &logger_head.list, list) {
		MFC_Print(handle, p->message);
	}
}

void screen_logger_empty(void)
{
	screen_logger *p = list_entry(logger_head.list.prev, typeof(*p), list);
	while (p != &logger_head) {
		screen_logger_remove_message(p->obj);
		p = list_entry(logger_head.list.prev, typeof(*p), list);
	}
}

void screen_logger_test_case(MFC_HANDLE handle)
{
	screen_logger_add_message("message1", MESSAGE_REPLACE, "print message1\n");
	screen_logger_add_message("message1", MESSAGE_REPLACE, "print message2\n");
	screen_logger_print(handle);
	screen_logger_empty();
}
