#ifndef _MSDC_IO_H_
#define _MSDC_IO_H_

/*--------------------------------------------------------------------------*/
/* MSDC0~3 GPIO and IO Pad Configuration Base                               */
/*--------------------------------------------------------------------------*/
#define MSDC_GPIO_BASE		gpio_base
#define MSDC0_IO_PAD_BASE	(msdc0_io_cfg_base)
#define MSDC1_IO_PAD_BASE	(msdc1_io_cfg_base)
#define MSDC2_IO_PAD_BASE	(msdc2_io_cfg_base)
#define MSDC3_IO_PAD_BASE	(msdc3_io_cfg_base)

/*--------------------------------------------------------------------------*/
/* MSDC GPIO Related Register                                               */
/*--------------------------------------------------------------------------*/
#define MSDC0_GPIO_MODE18	(MSDC_GPIO_BASE + 0x410)
#define MSDC0_GPIO_MODE19	(MSDC_GPIO_BASE + 0x420)
#define MSDC0_GPIO_IES_ADDR	(MSDC0_IO_PAD_BASE + 0x000)
#define MSDC0_GPIO_SMT_ADDR	(MSDC0_IO_PAD_BASE + 0x010)
#define MSDC0_GPIO_TDSEL_ADDR	(MSDC0_IO_PAD_BASE + 0x020)
#define MSDC0_GPIO_RDSEL_ADDR	(MSDC0_IO_PAD_BASE + 0x040)
#define MSDC0_GPIO_SR_ADDR	(MSDC0_IO_PAD_BASE + 0x0a0)
#define MSDC0_GPIO_DRV_ADDR	(MSDC0_IO_PAD_BASE + 0x0a0)
#define MSDC0_GPIO_PUPD0_ADDR	(MSDC0_IO_PAD_BASE + 0x0c0)
#define MSDC0_GPIO_PUPD1_ADDR	(MSDC0_IO_PAD_BASE + 0x0d0)

#define MSDC1_GPIO_MODE4	(MSDC_GPIO_BASE + 0x330)
#define MSDC1_GPIO_IES_ADDR	(MSDC1_IO_PAD_BASE + 0x000)
#define MSDC1_GPIO_SMT_ADDR	(MSDC1_IO_PAD_BASE + 0x010)
#define MSDC1_GPIO_TDSEL_ADDR	(MSDC1_IO_PAD_BASE + 0x030)
#define MSDC1_GPIO_RDSEL0_ADDR	(MSDC1_IO_PAD_BASE + 0x040)
#define MSDC1_GPIO_RDSEL1_ADDR	(MSDC1_IO_PAD_BASE + 0x050)
#define MSDC1_GPIO_SR_ADDR	(MSDC1_IO_PAD_BASE + 0x0b0)
#define MSDC1_GPIO_DRV_ADDR	(MSDC1_IO_PAD_BASE + 0x0b0)
#define MSDC1_GPIO_PUPD_ADDR	(MSDC1_IO_PAD_BASE + 0x0c0)

#define MSDC2_GPIO_MODE14	(MSDC_GPIO_BASE + 0x3D0)
#define MSDC2_GPIO_IES_ADDR	(MSDC2_IO_PAD_BASE + 0x000)
#define MSDC2_GPIO_SMT_ADDR	(MSDC2_IO_PAD_BASE + 0x010)
#define MSDC2_GPIO_TDSEL_ADDR	(MSDC2_IO_PAD_BASE + 0x020)
#define MSDC2_GPIO_RDSEL_ADDR	(MSDC2_IO_PAD_BASE + 0x040)
#define MSDC2_GPIO_SR_ADDR	(MSDC2_IO_PAD_BASE + 0x0a0)
#define MSDC2_GPIO_DRV_ADDR	(MSDC2_IO_PAD_BASE + 0x0a0)
#define MSDC2_GPIO_PUPD_ADDR	(MSDC2_IO_PAD_BASE + 0x0c0)

/*
 * MSDC0 GPIO and PAD register and bitfields definition
 */
/*MSDC0_GPIO_MODE19, Set as pinmux value as 1*/
#define MSDC0_MODE_DAT7_MASK            (7 << 25)
#define MSDC0_MODE_DAT6_MASK            (7 <<  9)
#define MSDC0_MODE_DAT5_MASK            (7 <<  6)
#define MSDC0_MODE_DAT4_MASK            (7 << 12)
#define MSDC0_MODE_DAT3_MASK            (7 << 22)
#define MSDC0_MODE_DAT2_MASK            (7 << 19)
#define MSDC0_MODE_DAT1_MASK            (7 <<  3)
#define MSDC0_MODE_CMD_MASK             (7 <<  0)
#define MSDC0_MODE_DSL_MASK             (7 << 28)
#define MSDC0_MODE_RST_MASK             (7 << 16)

/*MSDC0_GPIO_MODE18, Set as pinmux value as 1*/
#define MSDC0_MODE_DAT0_MASK            (7 << 25)
#define MSDC0_MODE_CLK_MASK             (7 << 28)

/* MSDC0 IES mask*/
#define MSDC0_IES_DAT_MASK              (0x1  <<  0)
#define MSDC0_IES_CLK_MASK              (0x1  <<  1)
#define MSDC0_IES_CMD_MASK              (0x1  <<  2)
#define MSDC0_IES_RSTB_MASK             (0x1  <<  3)
#define MSDC0_IES_DSL_MASK              (0x1  <<  4)
#define MSDC0_IES_ALL_MASK              (0x1F <<  0)

/* MSDC0 SMT mask*/
#define MSDC0_SMT_DAT_MASK              (0x1  <<  0)
#define MSDC0_SMT_CLK_MASK              (0x1  <<  1)
#define MSDC0_SMT_CMD_MASK              (0x1  <<  2)
#define MSDC0_SMT_RSTB_MASK             (0x1  <<  3)
#define MSDC0_SMT_DSL_MASK              (0x1  <<  4)
#define MSDC0_SMT_ALL_MASK              (0x1F <<  0)

/* MSDC0 TDSEL mask*/
#define MSDC0_TDSEL_DAT_MASK            (0xF  <<  0)
#define MSDC0_TDSEL_CLK_MASK            (0xF  <<  4)
#define MSDC0_TDSEL_CMD_MASK            (0xF  <<  8)
#define MSDC0_TDSEL_RSTB_MASK           (0xF  << 12)
#define MSDC0_TDSEL_DSL_MASK            (0xF  << 16)
#define MSDC0_TDSEL_ALL_MASK            (0xFFFFF << 0)

/* MSDC0 RDSEL mask*/
#define MSDC0_RDSEL_DAT_MASK            (0x3F <<  0)
#define MSDC0_RDSEL_CLK_MASK            (0x3F <<  6)
#define MSDC0_RDSEL_CMD_MASK            (0x3F << 12)
#define MSDC0_RDSEL_RSTB_MASK           (0x3F << 18)
#define MSDC0_RDSEL_DSL_MASK            (0x3F << 24)
#define MSDC0_RDSEL_ALL_MASK            (0x3FFFFFFF << 0)

/* MSDC0 SR mask*/
#define MSDC0_SR_DAT_MASK               (0x1  <<  3)
#define MSDC0_SR_CLK_MASK               (0x1  <<  7)
#define MSDC0_SR_CMD_MASK               (0x1  << 11)
#define MSDC0_SR_RSTB_MASK              (0x1  << 15)
#define MSDC0_SR_DSL_MASK               (0x1  << 19)
/*Attention: bits are not continuous, shall not define MSDC0_SR_ALL_MASK*/

/* MSDC0 DRV mask*/
#define MSDC0_DRV_DAT_MASK              (0x7  <<  0)
#define MSDC0_DRV_CLK_MASK              (0x7  <<  4)
#define MSDC0_DRV_CMD_MASK              (0x7  <<  8)
#define MSDC0_DRV_RSTB_MASK             (0x7  << 12)
#define MSDC0_DRV_DSL_MASK              (0x7  << 16)
/*Attention: bits are not continuous, shall not define MSDC0_DRV_ALL_MASK*/

/* MSDC0 PUPD mask*/
#define MSDC0_PUPD_DAT0_MASK            (0x7  <<  0)
#define MSDC0_PUPD_CLK_MASK             (0x7  <<  4)
#define MSDC0_PUPD_CMD_MASK             (0x7  <<  8)
#define MSDC0_PUPD_DAT1_MASK            (0x7  << 12)
#define MSDC0_PUPD_DAT5_MASK            (0x7  << 16)
#define MSDC0_PUPD_DAT6_MASK            (0x7  << 20)
#define MSDC0_PUPD_DAT4_MASK            (0x7  << 24)
#define MSDC0_PUPD_RSTB_MASK            (0x7  << 28)
#define MSDC0_PUPD0_MASK_WITH_RSTB      (0x77777777 << 0)
#define MSDC0_PUPD0_MASK                (0x7777777 << 0)

#define MSDC0_PUPD_DAT2_MASK            (0x7  <<  0)
#define MSDC0_PUPD_DAT3_MASK            (0x7  <<  4)
#define MSDC0_PUPD_DAT7_MASK            (0x7  <<  8)
#define MSDC0_PUPD_DSL_MASK             (0x7  << 12)
#define MSDC0_PUPD1_MASK                (0x7777 << 0)

/*
 * MSDC1 GPIO and PAD register and bitfields definition
 */
/*MSDC0_GPIO_MODE4, Set as pinmux value as 1*/
#define MSDC1_MODE_CLK_MASK             (7 <<  0)
#define MSDC1_MODE_CMD_MASK             (7 <<  6)
#define MSDC1_MODE_DAT0_MASK            (7 <<  9)
#define MSDC1_MODE_DAT1_MASK            (7 << 16)
#define MSDC1_MODE_DAT2_MASK            (7 << 12)
#define MSDC1_MODE_DAT3_MASK            (7 <<  3)

/* MSDC1 IES mask*/
#define MSDC1_IES_CLK_MASK              (0x1 <<  8)
#define MSDC1_IES_CMD_MASK              (0x1 <<  9)
#define MSDC1_IES_DAT_MASK              (0x1 << 10)
#define MSDC1_IES_ALL_MASK              (0x7 <<  8)

/* MSDC1 SMT mask*/
#define MSDC1_SMT_CLK_MASK              (0x1 <<  8)
#define MSDC1_SMT_CMD_MASK              (0x1 <<  9)
#define MSDC1_SMT_DAT_MASK              (0x1 << 10)
#define MSDC1_SMT_ALL_MASK              (0x7 <<  8)

/* MSDC1 TDSEL mask*/
#define MSDC1_TDSEL_CLK_MASK            (0xF <<  0)
#define MSDC1_TDSEL_CMD_MASK            (0xF <<  4)
#define MSDC1_TDSEL_DAT_MASK            (0xF <<  8)
#define MSDC1_TDSEL_ALL_MASK            (0xFFF << 0)

/* MSDC1 RDSEL mask*/
/*MSDC1_GPIO_RDSEL0*/
#define MSDC1_RDSEL_CLK_MASK            (0x3F << 16)
#define MSDC1_RDSEL_CMD_MASK            (0x3F << 22)
/*MSDC1_GPIO_RDSEL1*/
#define MSDC1_RDSEL_DAT_MASK            (0x3F <<  0)
#define MSDC1_RDSEL0_ALL_MASK           (0xFFF << 16)
#define MSDC1_RDSEL1_ALL_MASK           (0x3F <<  0)
/*Attention: not the same address, shall not define MSDC1_RDSEL_ALL_MASK*/

/* MSDC1 SR mask*/
#define MSDC1_SR_CLK_MASK               (0x1 <<  3)
#define MSDC1_SR_CMD_MASK               (0x1 <<  7)
#define MSDC1_SR_DAT_MASK               (0x1 << 11)
/*Attention: bits are not continuous, shall not define MSDC1_SR_ALL_MASK*/

/* MSDC1 DRV mask*/
#define MSDC1_DRV_CLK_MASK              (0x7 <<  0)
#define MSDC1_DRV_CMD_MASK              (0x7 <<  4)
#define MSDC1_DRV_DAT_MASK              (0x7 <<  8)
/*Attention: bits are not continuous, shall not define MSDC1_DRV_ALL_MASK*/

/* MSDC1 PUPD mask*/
#define MSDC1_PUPD_CMD_MASK             (0x7  <<  0)
#define MSDC1_PUPD_CLK_MASK             (0x7  <<  4)
#define MSDC1_PUPD_DAT0_MASK            (0x7  <<  8)
#define MSDC1_PUPD_DAT1_MASK            (0x7  << 12)
#define MSDC1_PUPD_DAT2_MASK            (0x7  << 16)
#define MSDC1_PUPD_DAT3_MASK            (0x7  << 20)
#define MSDC1_PUPD_MASK                 (0x777777 << 0)

/*
 * MSDC2 GPIO and PAD register and bitfields definition
 */
/*MSDC2_GPIO_MODE14, Set as pinmux value as 2*/
#define MSDC2_MODE_CLK_MASK             (7 <<  9)
#define MSDC2_MODE_CMD_MASK             (7 <<  6)
#define MSDC2_MODE_DAT0_MASK            (7 << 16)
#define MSDC2_MODE_DAT1_MASK            (7 <<  3)
#define MSDC2_MODE_DAT2_MASK            (7 << 19)
#define MSDC2_MODE_DAT3_MASK            (7 << 12)

/* MSDC2 IES mask*/
#define MSDC2_IES_CLK_MASK              (0x1 << 5)
#define MSDC2_IES_CMD_MASK              (0x1 << 4)
#define MSDC2_IES_DAT_MASK              (0x1 << 3)
#define MSDC2_IES_ALL_MASK              (0x7 << 3)

/* MSDC2 SMT mask*/
#define MSDC2_SMT_CLK_MASK              (0x1 << 5)
#define MSDC2_SMT_CMD_MASK              (0x1 << 4)
#define MSDC2_SMT_DAT_MASK              (0x1 << 3)
#define MSDC2_SMT_ALL_MASK              (0x7 << 3)

/* MSDC2 TDSEL mask*/
#define MSDC2_TDSEL_CLK_MASK            (0xF << 20)
#define MSDC2_TDSEL_CMD_MASK            (0xF << 16)
#define MSDC2_TDSEL_DAT_MASK            (0xF << 12)
#define MSDC2_TDSEL_ALL_MASK            (0xFFF << 12)

/* MSDC2 RDSEL mask*/
#define MSDC2_RDSEL_CLK_MASK            (0x3 << 10)
#define MSDC2_RDSEL_CMD_MASK            (0x3 << 8)
#define MSDC2_RDSEL_DAT_MASK            (0x3 << 6)
#define MSDC2_RDSEL_ALL_MASK            (0x3F << 6)

/* MSDC2 SR mask*/
#define MSDC2_SR_CLK_MASK               (0x1 << 23)
#define MSDC2_SR_CMD_MASK               (0x1 << 19)
#define MSDC2_SR_DAT_MASK               (0x1 << 15)

/* MSDC2 DRV mask*/
#define MSDC2_DRV_CLK_MASK              (0x7 << 20)
#define MSDC2_DRV_CMD_MASK              (0x7 << 16)
#define MSDC2_DRV_DAT_MASK              (0x7 << 12)

/* MSDC2 PUPD mask*/
#define MSDC2_PUPD_CLK_MASK             (0x7 << 20)
#define MSDC2_PUPD_CMD_MASK             (0x7 << 16)
#define MSDC2_PUPD_DAT_MASK             (0x7 << 12)
#define MSDC2_PUPD_MASK                 (0xFFF << 12)

#endif /* end of _MSDC_IO_H_ */
