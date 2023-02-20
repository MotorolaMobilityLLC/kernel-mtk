#! /usr/bin/python
# -*- coding: utf-8 -*-

# Copyright Statement:
#
# This software/firmware and related documentation ("MediaTek Software") are
# protected under relevant copyright laws. The information contained herein is
# confidential and proprietary to MediaTek Inc. and/or its licensors. Without
# the prior written permission of MediaTek inc. and/or its licensors, any
# reproduction, modification, use or disclosure of MediaTek Software, and
# information contained herein, in whole or in part, shall be strictly
# prohibited.
#
# MediaTek Inc. (C) 2019. All rights reserved.
#
# BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
# THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
# RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
# ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
# WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
# NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
# RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
# INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
# TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
# RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
# OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
# SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
# RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
# STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
# ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
# RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
# MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
# CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
#
# The following software/firmware and/or related documentation ("MediaTek
# Software") have been modified by MediaTek Inc. All revisions are subject to
# any receiver's applicable license agreements with MediaTek Inc.	

import os, sys
import getopt
import traceback
import subprocess
import xml.dom.minidom

sys.dont_write_bytecode = True

sys.path.append('.')
sys.path.append('..')

from obj.ChipObj import ChipObj
from obj.ChipObj import MT6797
from obj.ChipObj import MT6757
from obj.ChipObj import MT6757_P25
from obj.ChipObj import MT6570
from obj.ChipObj import MT6799
from obj.ChipObj import MT6759
from obj.ChipObj import MT6763
from obj.ChipObj import MT6750S
from obj.ChipObj import MT6758
from obj.ChipObj import MT6739
from obj.ChipObj import MT8695
from obj.ChipObj import MT6771
from obj.ChipObj import MT6775
from obj.ChipObj import MT6779
from obj.ChipObj import MT6768
from obj.ChipObj import MT6785
from obj.ChipObj import MT6885
from obj.ChipObj import MT6853
from obj.ChipObj import MT6781
from obj.ChipObj import MT6879

from utility.util import LogLevel
from utility.util import log

def usage():
    print '''
usage: DrvGen [dws_path] [file_path] [log_path] [paras]...

options and arguments:

dws_path    :    dws file path
file_path   :    where you want to put generated files
log_path    :    where to store the log files
paras        :    parameter for generate wanted file
'''

def is_oldDws(path, gen_spec):
    if not os.path.exists(path):
        log(LogLevel.error, 'Can not find %s' %(path))
        sys.exit(-1)

    try:
        root = xml.dom.minidom.parse(dws_path)
    except Exception, e:
        log(LogLevel.warn, '%s is not xml format, try to use old DCT!' %(dws_path))
        if len(gen_spec) == 0:
            log(LogLevel.warn, 'Please use old DCT UI to gen all files!')
            return True
        #old_dct = os.path.join(sys.path[0], 'old_dct', 'DrvGen')
        drvgen_path = sys.argv[0]
        parent_path = os.path.abspath(os.path.dirname(drvgen_path) + os.path.sep + "..")
        old_dct = os.path.join(parent_path, 'old_dct', 'DrvGen')
        log(LogLevel.info, "old dct path: %s" % (old_dct))
        cmd = old_dct + ' ' + dws_path + ' ' + gen_path + ' ' + log_path + ' ' + gen_spec[0]
        log(LogLevel.info, "old dct cmd: %s" % (cmd))

        if 0 == subprocess.call(cmd, shell=True):
            return True
        else:
            log(LogLevel.error, '%s format error!' %(dws_path))
            sys.exit(-1)

    return False

if __name__ == '__main__':
    opts, args = getopt.getopt(sys.argv[1:], '')

    if len(args) == 0:
        msg = 'Too less arguments!'
        usage()
        log(LogLevel.error, msg)
        sys.exit(-1)

    dws_path = ''
    gen_path = ''
    log_path = ''
    gen_spec = []

    # get DWS file path from parameters
    dws_path = os.path.abspath(args[0])

    # get parameters from input
    if len(args) == 1:
        gen_path = os.path.dirname(dws_path)
        log_path = os.path.dirname(dws_path)

    elif len(args) == 2:
        gen_path = os.path.abspath(args[1])
        log_path = os.path.dirname(dws_path)

    elif len(args) == 3:
        gen_path = os.path.abspath(args[1])
        log_path = os.path.abspath(args[2])

    elif len(args) >= 4:
        gen_path = os.path.abspath(args[1])
        log_path = os.path.abspath(args[2])
        for i in range(3,len(args)):
            gen_spec.append(args[i])

    log(LogLevel.info, 'DWS file path is %s' %(dws_path))
    log(LogLevel.info, 'Gen files path is %s' %(gen_path))
    log(LogLevel.info, 'Log files path is %s' %(log_path))

    for item in gen_spec:
        log(LogLevel.info, 'Parameter is %s' %(item))



    # check DWS file path
    if not os.path.exists(dws_path):
        log(LogLevel.error, 'Can not find "%s", file not exist!' %(dws_path))
        sys.exit(-1)

    if not os.path.exists(gen_path):
        log(LogLevel.error, 'Can not find "%s", gen path not exist!' %(gen_path))
        sys.exit(-1)

    if not os.path.exists(log_path):
        log(LogLevel.error, 'Can not find "%s", log path not exist!' %(log_path))
        sys.exit(-1)

    if is_oldDws(dws_path, gen_spec):
        sys.exit(0)

    chipId = ChipObj.get_chipId(dws_path)
    log(LogLevel.info, 'chip id: %s' %(chipId))
    chipObj = None
    if cmp(chipId, 'MT6797') == 0:
        chipObj = MT6797(dws_path, gen_path)
    elif cmp(chipId, 'MT6757') == 0:
        chipObj = MT6757(dws_path, gen_path)
    elif cmp(chipId, 'MT6757-P25') == 0:
        chipObj = MT6757_P25(dws_path, gen_path)
    elif cmp(chipId, 'MT6570') == 0:
        chipObj = MT6570(dws_path, gen_path)
    elif cmp(chipId, 'MT6799') == 0:
        chipObj = MT6799(dws_path, gen_path)
    elif cmp(chipId, 'MT6763') == 0:
        chipObj = MT6763(dws_path, gen_path)
    elif cmp(chipId, 'MT6759') == 0:
        chipObj = MT6759(dws_path, gen_path)
    elif cmp(chipId, 'MT6750S') == 0:
        chipObj = MT6750S(dws_path, gen_path)
    elif cmp(chipId, 'MT6758') == 0:
        chipObj = MT6758(dws_path, gen_path)
    elif cmp(chipId, 'MT6739') == 0:
        chipObj = MT6739(dws_path, gen_path)
    elif cmp(chipId, 'MT8695') == 0 or \
         cmp(chipId, 'MT8168') == 0 or \
         cmp(chipId, 'MT8696') == 0 or \
         cmp(chipId, 'MT8169') == 0:
        chipObj = MT8695(dws_path, gen_path)
    elif cmp(chipId, 'MT6771') == 0 or \
         cmp(chipId, 'MT6775') == 0 or \
         cmp(chipId, 'MT6765') == 0 or \
         cmp(chipId, 'MT3967') == 0 or \
         cmp(chipId, 'MT6761') == 0:
        chipObj = MT6771(dws_path, gen_path)
    elif cmp(chipId, 'MT6779') == 0:
        chipObj = MT6779(dws_path, gen_path)
    elif cmp(chipId, 'MT6768') == 0:
        chipObj = MT6768(dws_path, gen_path)
    elif cmp(chipId, 'MT6785') == 0:
        chipObj = MT6785(dws_path, gen_path)
    elif cmp(chipId, 'MT6885') == 0 or \
         cmp(chipId, 'MT6873') == 0 or \
         cmp(chipId, 'MT6893') == 0:
        chipObj = MT6885(dws_path, gen_path)
    elif cmp(chipId, 'MT6853') == 0 or \
         cmp(chipId, 'MT6880') == 0 or \
         cmp(chipId, 'MT6833') == 0 or \
         cmp(chipId, 'MT6877') == 0:
        chipObj = MT6853(dws_path, gen_path)
    elif cmp(chipId, 'MT6781') == 0:
        chipObj = MT6781(dws_path, gen_path)
    elif cmp(chipId, 'MT6879') == 0 or \
         cmp(chipId, 'MT6983') == 0 or \
         cmp(chipId, 'MT6895') == 0:
        chipObj = MT6879(dws_path, gen_path)
    else:
        chipObj = ChipObj(dws_path, gen_path)

    if not chipObj.parse():
        log(LogLevel.error, 'Parse %s fail!' %(dws_path))
        sys.exit(-1)

    if not chipObj.generate(gen_spec):
        log(LogLevel.error, 'Generate files fail!')
        sys.exit(-1)

    sys.exit(0)

