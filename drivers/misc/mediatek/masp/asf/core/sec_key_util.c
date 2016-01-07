#include "sec_typedef.h"
#include "sec_boot.h"

/**************************************************************************
 *  DEFINITIONS
 **************************************************************************/
#define MOD                         "SEC_KEY_UTIL"

/**************************************************************************
 *  KEY SECRET
 **************************************************************************/
#define ENCODE_MAGIC                (0x1)

void sec_decode_key(unsigned char *key, unsigned int key_len, unsigned char *seed, unsigned int seed_len)
{
	unsigned int i = 0;

	for (i = 0; i < key_len; i++) {
		key[i] -= seed[i % seed_len];
		key[i] -= ENCODE_MAGIC;
	}
}
