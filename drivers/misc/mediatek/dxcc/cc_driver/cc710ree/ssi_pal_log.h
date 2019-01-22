/*****************************************************************************
* Copyright (C) 2015 ARM Limited or its affiliates.	                     *
* This program is free software; you can redistribute it and/or modify it    *
* under the terms of the GNU General Public License as published by the Free *
* Software Foundation; either version 2 of the License, or (at your option)  * 
* any later version.							     *
* This program is distributed in the hope that it will be useful, but 	     *
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
* or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
* for more details.							     *	
* You should have received a copy of the GNU General Public License along    *
* with this program; if not, write to the Free Software Foundation, 	     *
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.        *
******************************************************************************/

#ifndef _SSI_PAL_LOG_H_
#define _SSI_PAL_LOG_H_

#include "ssi_pal_types.h"
#include "ssi_pal_log_plat.h"

/*!
@file
@brief This file contains the PAL layer log definitions, by default the log is disabled.
*/

/* PAL log levels (to be used in SASI_PAL_logLevel) */
#define SASI_PAL_LOG_LEVEL_NULL      -1 /*!< \internal Disable logging */
#define SASI_PAL_LOG_LEVEL_ERR       0
#define SASI_PAL_LOG_LEVEL_WARN      1
#define SASI_PAL_LOG_LEVEL_INFO      2
#define SASI_PAL_LOG_LEVEL_DEBUG     3
#define SASI_PAL_LOG_LEVEL_TRACE     4
#define SASI_PAL_LOG_LEVEL_DATA      5

#ifndef SASI_PAL_LOG_CUR_COMPONENT
/* Setting default component mask in case caller did not define */
/* (a mask that is always on for every log mask value but full masking) */
#define SASI_PAL_LOG_CUR_COMPONENT 0xFFFFFFFF
#endif
#ifndef SASI_PAL_LOG_CUR_COMPONENT_NAME
#define SASI_PAL_LOG_CUR_COMPONENT_NAME "Dx"
#endif

/* Select compile time log level (default if not explicitly specified by caller) */
#ifndef SASI_PAL_MAX_LOG_LEVEL /* Can be overriden by external definition of this constant */
#ifdef DEBUG
#define SASI_PAL_MAX_LOG_LEVEL  SASI_PAL_LOG_LEVEL_ERR /*SASI_PAL_LOG_LEVEL_DEBUG*/
#else /* Disable logging */
#define SASI_PAL_MAX_LOG_LEVEL SASI_PAL_LOG_LEVEL_NULL
#endif
#endif /*SASI_PAL_MAX_LOG_LEVEL*/
/* Evaluate SASI_PAL_MAX_LOG_LEVEL in case provided by caller */
#define __SASI_PAL_LOG_LEVEL_EVAL(level) level
#define _SASI_PAL_MAX_LOG_LEVEL __SASI_PAL_LOG_LEVEL_EVAL(SASI_PAL_MAX_LOG_LEVEL)


#ifdef ARM_DSM
#define SaSi_PalLogInit() do {} while (0)
#define SaSi_PalLogLevelSet(setLevel) do {} while (0)
#define SaSi_PalLogMaskSet(setMask) do {} while (0)
#else
#if _SASI_PAL_MAX_LOG_LEVEL > SASI_PAL_LOG_LEVEL_NULL
void SaSi_PalLogInit(void);
void SaSi_PalLogLevelSet(int setLevel);
void SaSi_PalLogMaskSet(uint32_t setMask);
extern int SaSi_PAL_logLevel;
extern uint32_t SaSi_PAL_logMask;
#else /* No log */
static inline void SaSi_PalLogInit(void) {}
static inline void SaSi_PalLogLevelSet(int setLevel) {SASI_UNUSED_PARAM(setLevel);}
static inline void SaSi_PalLogMaskSet(uint32_t setMask) {SASI_UNUSED_PARAM(setMask);}
#endif
#endif

/* Filter logging based on logMask and dispatch to platform specific logging mechanism */
#define _SASI_PAL_LOG(level, format, ...)  \
	if (SaSi_PAL_logMask & SASI_PAL_LOG_CUR_COMPONENT) \
		__SASI_PAL_LOG_PLAT(SASI_PAL_LOG_LEVEL_ ## level, "%s:%s: " format, SASI_PAL_LOG_CUR_COMPONENT_NAME, __func__, ##__VA_ARGS__)

#if (_SASI_PAL_MAX_LOG_LEVEL >= SASI_PAL_LOG_LEVEL_ERR)
#define SASI_PAL_LOG_ERR(format, ... ) \
	_SASI_PAL_LOG(ERR, format, ##__VA_ARGS__)
#else
#define SASI_PAL_LOG_ERR( ... ) do {} while (0)
#endif

#if (_SASI_PAL_MAX_LOG_LEVEL >= SASI_PAL_LOG_LEVEL_WARN)
#define SASI_PAL_LOG_WARN(format, ... ) \
	if (SaSi_PAL_logLevel >= SASI_PAL_LOG_LEVEL_WARN) \
		_SASI_PAL_LOG(WARN, format, ##__VA_ARGS__)
#else
#define SASI_PAL_LOG_WARN( ... ) do {} while (0)
#endif

#if (_SASI_PAL_MAX_LOG_LEVEL >= SASI_PAL_LOG_LEVEL_INFO)
#define SASI_PAL_LOG_INFO(format, ... ) \
	if (SaSi_PAL_logLevel >= SASI_PAL_LOG_LEVEL_INFO) \
		_SASI_PAL_LOG(INFO, format, ##__VA_ARGS__)
#else
#define SASI_PAL_LOG_INFO( ... ) do {} while (0)
#endif

#if (_SASI_PAL_MAX_LOG_LEVEL >= SASI_PAL_LOG_LEVEL_DEBUG)
#define SASI_PAL_LOG_DEBUG(format, ... ) \
	if (SaSi_PAL_logLevel >= SASI_PAL_LOG_LEVEL_DEBUG) \
		_SASI_PAL_LOG(DEBUG, format, ##__VA_ARGS__)

#define SASI_PAL_LOG_DUMP_BUF(msg, buf, size)		\
	do {						\
	int i;						\
	uint8_t	*pData = (uint8_t*)buf;			\
							\
	PRINTF("%s (%d):\n", msg, size);		\
	for (i = 0; i < size; i++) {			\
		PRINTF("0x%02X ", pData[i]);		\
		if ((i & 0xF) == 0xF) {			\
			PRINTF("\n");			\
		}					\
	}						\
	PRINTF("\n");					\
	} while (0)
#else
#define SASI_PAL_LOG_DEBUG( ... ) do {} while (0)
#define SASI_PAL_LOG_DUMP_BUF(msg, buf, size)	do {} while (0)
#endif

#if (_SASI_PAL_MAX_LOG_LEVEL >= SASI_PAL_LOG_LEVEL_TRACE)
#define SASI_PAL_LOG_TRACE(format, ... ) \
	if (SaSi_PAL_logLevel >= SASI_PAL_LOG_LEVEL_TRACE) \
		_SASI_PAL_LOG(TRACE, format, ##__VA_ARGS__)
#else
#define SASI_PAL_LOG_TRACE(...) do {} while (0)
#endif

#if (_SASI_PAL_MAX_LOG_LEVEL >= SASI_PAL_LOG_LEVEL_TRACE)
#define SASI_PAL_LOG_DATA(format, ...) \
	if (SaSi_PAL_logLevel >= SASI_PAL_LOG_LEVEL_TRACE) \
		_SASI_PAL_LOG(DATA, format, ##__VA_ARGS__)
#else
#define SASI_PAL_LOG_DATA( ...) do {} while (0)
#endif

#endif /*_SSI_PAL_LOG_H_*/
