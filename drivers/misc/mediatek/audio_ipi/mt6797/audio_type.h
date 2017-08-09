#ifndef AUDIO_TYPE_H
#define AUDIO_TYPE_H

#include "audio_log.h"
#include "audio_assert.h"

typedef enum {
	NO_ERROR,
	UNKNOWN_ERROR,

	NO_MEMORY,
	NO_INIT,
	NOT_ENOUGH_DATA,

	ALREADY_EXISTS,

	BAD_INDEX,
	BAD_VALUE,
	BAD_TYPE,

	TIMED_OUT,
} audio_status_t;



#endif /* end of AUDIO_TYPE_H */
