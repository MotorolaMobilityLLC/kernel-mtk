#!/bin/bash

my_top_dir=$PWD

mkdir -vp $my_top_dir/kernel-6.6/prebuilts/


cd $my_top_dir/kernel-6.6/prebuilts/


git clone https://android.googlesource.com/kernel/prebuilts/build-tools


mkdir -vp $my_top_dir/prebuilts/clang/host/


cd $my_top_dir/prebuilts/clang/host/


git clone https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86


mkdir -vp $my_top_dir/prebuilts/gcc/linux-x86/host


cd $my_top_dir/prebuilts/gcc/linux-x86/host


git clone https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.17-4.8


cd $my_top_dir/prebuilts/


git clone https://android.googlesource.com/platform/prebuilts/clang-tools


cd $my_top_dir/prebuilts/


git clone -b android-u-beta-5.3_r0.5 --single-branch https://android.googlesource.com/kernel/prebuilts/build-tools kernel-build-tools


cd $my_top_dir/prebuilts/


git clone -b ndk-r23 --single-branch https://android.googlesource.com/platform/ndk ndk-r23


mkdir -vp $my_top_dir/kernel-6.6/


cd $my_top_dir/kernel-6.6


mkdir -vp $my_top_dir/kernel-6.6/tools


Download kernel source code. Rename kernel-6.6 folder to $my_top_dir/kernel/kernel_device_modules-6.6

KERNEL_DIR=$PWD/kernel/kernel_device_modules-6.6

KERNEL_MODULES_DIR=$PWD/vendor/mediatek/kernel_modules

REL_KERNEL_OUT="out/target/product/lamu/obj/KERNEL_OBJ"

kernel_out_dir=$my_top_dir/out/target/product/lamu/obj/KERNEL_OBJ/kernel-6.6

MODULES_STAGING_DIR=$my_top_dir/out/target/product/lamu/obj/KERNEL_OBJ/staging

KERNEL_ZIMAGE_OUT="$kernel_out_dir/arch/arm64/boot/Image.gz"

TARGET_KERNEL_CONFIG="$kernel_out_dir/.config"

PATH=$my_top_dir/kernel/build/build-tools/path/linux-x86:$my_top_dir/kernel/prebuilts-master/clang/host/linux-x86/clang-r416183b/bin:$my_top_dir/prebuilts/perl/linux-x86/bin:$my_top_dir/kernel/prebuilts/kernel-build-tools/linux-x86/bin:$PATH

export CLANG_TRIPLE= CROSS_COMPILE=aarch64-linux-gnu- CROSS_COMPILE_COMPAT=arm-linux-gnueabi- CROSS_COMPILE_ARM32= ARCH=arm64 SUBARCH= MAKE_GOALS=all HOSTCC=clang HOSTCXX=clang++ CC=clang LD=ld.lld AR=llvm-ar NM=llvm-nm OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump READELF=llvm-readelf OBJSIZE=llvm-size STRIP=llvm-strip

mkdir -vp $kernel_out_dir

cd $KERNEL_DIR

python $KERNEL_DIR/scripts/gen_build_config.py --kernel-defconfig lamu_defconfig -m user -o $my_top_dir/out/target/product/lamu/obj/KERNEL_OBJ/build.config

cp -p $KERNEL_DIR/arch/arm64/configs/lamu_defconfig $my_top_dir/out/target/product/lamu/obj/KERNEL_OBJ/lamu.config

make LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc O=$kernel_out_dir gki_defconfig ../../../../out/target/product/lamu/obj/KERNEL_OBJ/lamu.config

make -j48 O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc all vmlinux

make -j48 -C $kernel_out_dir O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

#build modules
make -C $KERNEL_MODULES_DIR/connectivity/common M=$KERNEL_MODULES_DIR/connectivity/common src=$KERNEL_MODULES_DIR/connectivity/common KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/common M=$KERNEL_MODULES_DIR/connectivity/common src=$KERNEL_MODULES_DIR/connectivity/common KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/connectivity/conninfra M=$KERNEL_MODULES_DIR/connectivity/conninfra src=$KERNEL_MODULES_DIR/connectivity/conninfra KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/conninfra M=$KERNEL_MODULES_DIR/connectivity/conninfra src=$KERNEL_MODULES_DIR/connectivity/conninfra KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/connectivity/connfem M=$KERNEL_MODULES_DIR/connectivity/connfem src=$KERNEL_MODULES_DIR/connectivity/connfem KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/connfem M=$KERNEL_MODULES_DIR/connectivity/connfem src=$KERNEL_MODULES_DIR/connectivity/connfem KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/connectivity/fmradio M=$KERNEL_MODULES_DIR/connectivity/fmradio src=$KERNEL_MODULES_DIR/connectivity/fmradio KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/fmradio M=$KERNEL_MODULES_DIR/connectivity/fmradio src=$KERNEL_MODULES_DIR/connectivity/fmradio KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr src=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr src=$KERNEL_MODULES_DIR/connectivity/gps/gps_pwr KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $KERNEL_MODULES_DIR/connectivity/gps/gps_stp M=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp src=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/connectivity/gps/gps_stp M=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp src=$KERNEL_MODULES_DIR/connectivity/gps/gps_stp KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor src=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc CONNAC_VER=1_0 MODULE_NAME=wmt_chrdev_wifi KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor src=$KERNEL_MODULES_DIR/connectivity/wlan/adaptor KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc CONNAC_VER=1_0 MODULE_NAME=wmt_chrdev_wifi KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m src=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc MODULE_NAME=wlan_drv_gen4m CONFIG_MTK_COMBO_WIFI_HIF=axi MODULE_NAME=wlan_drv_gen4m MTK_COMBO_CHIP=CONNAC WLAN_CHIP_ID=6768 MTK_ANDROID_WMT=y WIFI_ENABLE_GCOV= WIFI_IP_SET=1 MTK_ANDROID_EMI=y MTK_WLAN_SERVICE=yes KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/wlan/adaptor/Module.symvers $KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m src=$KERNEL_MODULES_DIR/connectivity/wlan/core/gen4m KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc MODULE_NAME=wlan_drv_gen4m CONFIG_MTK_COMBO_WIFI_HIF=axi MODULE_NAME=wlan_drv_gen4m MTK_COMBO_CHIP=CONNAC WLAN_CHIP_ID=6768 MTK_ANDROID_WMT=y WIFI_ENABLE_GCOV= WIFI_IP_SET=1 MTK_ANDROID_EMI=y MTK_WLAN_SERVICE=yes KBUILD_EXTRA_SYMBOLS="$KERNEL_MODULES_DIR/connectivity/wlan/adaptor/Module.symvers $KERNEL_MODULES_DIR/connectivity/conninfra/Module.symvers $KERNEL_MODULES_DIR/connectivity/common/Module.symvers" AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR modules_install

make -C $KERNEL_MODULES_DIR/fpsgo_cus M=$KERNEL_MODULES_DIR/fpsgo_cus src=$KERNEL_MODULES_DIR/fpsgo_cus KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/fpsgo_cus M=$KERNEL_MODULES_DIR/fpsgo_cus src=$KERNEL_MODULES_DIR/fpsgo_cus KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/gpu M=$KERNEL_MODULES_DIR/gpu src=$KERNEL_MODULES_DIR/gpu KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/gpu M=$KERNEL_MODULES_DIR/gpu src=$KERNEL_MODULES_DIR/gpu KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $KERNEL_MODULES_DIR/met_drv_v3 M=$KERNEL_MODULES_DIR/met_drv_v3 src=$KERNEL_MODULES_DIR/met_drv_v3 KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $KERNEL_MODULES_DIR/met_drv_v3 M=$KERNEL_MODULES_DIR/met_drv_v3 src=$KERNEL_MODULES_DIR/met_drv_v3 KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/udc src=$KERNEL_MODULES_DIR/udc KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h
make -C $kernel_out_dir M=$KERNEL_MODULES_DIR/udc src=$KERNEL_MODULES_DIR/udc KERNEL_SRC=$KERNEL_DIR O=$kernel_out_dir LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc AUTOCONF_H=$kernel_out_dir/include/generated/autoconf.h INSTALL_MOD_PATH=$MODULES_STAGING_DIR  modules_install

