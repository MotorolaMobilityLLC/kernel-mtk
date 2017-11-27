#!/bin/bash

# Mysteryagr
# Compile kernel with a build script to make things simple

# Clone your toolchain first
# git clone https://github.com/ArchiDroid/Toolchain.git -b linaro-4.9 toolchain

export CROSS_COMPILE=~/toolchain/bin/arm-eabi-

mkdir -p out

export USE_CCACHE=1

export ARCH=arm ARCH_MTK_PLATFORM=MT6735M

#Defconfig for Moto C
make -C $PWD O=$PWD/out ARCH=arm wt6737m_35_n_defconfig
#make ARCH=arm wt6737m_35_n_defconfig

make -j4 -C $PWD O=$PWD/out ARCH=arm
#make -j4 ARCH=arm
