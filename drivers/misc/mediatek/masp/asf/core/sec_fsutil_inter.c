/******************************************************************************
 *  INCLUDE LIBRARY
 ******************************************************************************/
#include "sec_boot_lib.h"

/**************************************************************************
 *  MODULE NAME
 **************************************************************************/
#define MOD                         "ASF.FS"

/**************************************************************************
 *  READ SECRO
 **************************************************************************/
unsigned int sec_fs_read_secroimg(char *path, char *buf)
{
	unsigned int ret = SEC_OK;
	const unsigned int size = sizeof(AND_SECROIMG_T);
	unsigned int temp = 0;
	ASF_FILE fd;

	/* ------------------------ */
	/* check parameter          */
	/* ------------------------ */
	SMSG(TRUE, "[%s] open '%s'\n", MOD, path);
	if (0 == size) {
		ret = ERR_FS_SECRO_READ_SIZE_CANNOT_BE_ZERO;
		goto _end;
	}

	/* ------------------------ */
	/* open secro               */
	/* ------------------------ */
	fd = ASF_OPEN(path);

	if (ASF_IS_ERR(fd)) {
		ret = ERR_FS_SECRO_OPEN_FAIL;
		goto _open_fail;
	}

	/* ------------------------ */
	/* read secro               */
	/* ------------------------ */
	/* configure file system type */
	osal_set_kernel_fs();

	/* adjust read off */
	ASF_SEEK_SET(fd, 0);

	/* read secro content */
	temp = ASF_READ(fd, buf, size);
	if (0 >= temp) {
		ret = ERR_FS_SECRO_READ_FAIL;
		goto _end;
	}

	if (size != temp) {
		SMSG(TRUE, "[%s] size '0x%x', read '0x%x'\n", MOD, size, temp);
		ret = ERR_FS_SECRO_READ_WRONG_SIZE;
		goto _end;
	}

	/* ------------------------ */
	/* check integrity          */
	/* ------------------------ */
	ret = sec_secro_check();
	if (SEC_OK != ret)
		goto _end;

	/* ------------------------ */
	/* SECROIMG is valid        */
	/* ------------------------ */
	bSecroExist = TRUE;

_end:
	ASF_CLOSE(fd);
	osal_restore_fs();

_open_fail:
	return ret;
}
