/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FBT_ERROR_H__
#define __FBT_ERROR_H__

enum FBT_ERROR {
	FBT_OK,
	FBT_ERROR_FAIL,
	FBT_ERROR_OOM,
	FBT_ERROR_OUT_OF_FD,
	FBT_ERROR_FAIL_WITH_LIMIT,
	FBT_ERROR_TIMEOUT,
	FBT_ERROR_CMD_NOT_PROCESSED,
	FBT_ERROR_INVALID_PARAMS,
	FBT_INTENTIONAL_BLOCK
};

#endif
