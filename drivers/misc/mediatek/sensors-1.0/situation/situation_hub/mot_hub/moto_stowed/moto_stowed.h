/*============================================================================
  @file moto_stowed.h

 Copyright (c) 2016 by Motorola Mobility, LLC.  All Rights Reserved
  ===========================================================================*/

#ifndef MOTO_STOWED_H
#define MOTO_STOWED_H

#include <linux/ioctl.h>

int __init moto_stowed_init(void);
void __exit moto_stowed_exit(void);

#endif
