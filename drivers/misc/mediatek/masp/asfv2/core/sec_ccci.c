#include "sec_hal.h"
#include "sec_boot_lib.h"
#include "sec_ccci.h"

/**************************************************************************
 *  MODULE NAME
 **************************************************************************/
#define MOD                         "SEC_CCCI"

void masp_secure_algo(unsigned char Direction, unsigned char *ContentAddr, unsigned int ContentLen,
		      unsigned char *CustomSeed, unsigned char *ResText)
{
	return masp_hal_secure_algo(Direction, ContentAddr, ContentLen, CustomSeed, ResText);
}

/* return the result of hwEnableClock ( )
   - TRUE  (1) means crypto engine init success
   - FALSE (0) means crypto engine init fail    */
unsigned char masp_secure_algo_init(void)
{
	return masp_hal_secure_algo_init();
}

/* return the result of hwDisableClock ( )
   - TRUE  (1) means crypto engine de-init success
   - FALSE (0) means crypto engine de-init fail    */
unsigned char masp_secure_algo_deinit(void)
{
	return masp_hal_secure_algo_deinit();
}
