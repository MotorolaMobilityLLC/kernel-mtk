/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2019 Google LLC
 */

#ifndef _UFSHCD_MOTO_CRYPTO_H
#define _UFSHCD_MOTO_CRYPTO_H

#include "ufshcd.h"
#include "ufshci.h"

int ufshcd_moto_hba_init_crypto_capabilities(struct ufs_hba *hba);

#endif
