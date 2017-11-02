cd kernel-3.18
mkdir -p ../out/target/product/$PRODUCT/obj/KERNEL_OBJ
make O=../out/target/product/$PRODUCT/obj/KERNEL_OBJ  ARCH=arm A158_debug_defconfig
make -j16 O=../out/target/product/$PRODUCT/obj/KERNEL_OBJ ARCH=arm CROSS_COMPILE=/home/name/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi- 2>&1 | tee ../k.log