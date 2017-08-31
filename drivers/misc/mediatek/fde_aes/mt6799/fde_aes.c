#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/proc_fs.h>
#include <linux/mmc/mmc.h>
#include <crypto/hash.h>
#include <mt-plat/mtk_secure_api.h>
#include "fde_aes.h"

static mt_fde_aes_context fde_aes_context;

#if FDE_AES_PROC
static s32 fde_aes_dump_reg(void)
{
	void __iomem *FDE_AES_CORE_BASE = fde_aes_context.base;

	if (!fde_aes_context.base)
		return -FDE_ENODEV;

	FDEERR(" ################################ FDE_AES DUMP START (Kernel) ################################\n");

	FDEERR("CONTEXT_SEL   %p = %x\n", CONTEXT_SEL, FDE_READ32(CONTEXT_SEL));
	FDEERR("CONTEXT_WORD0 %p = %x\n", CONTEXT_WORD0, FDE_READ32(CONTEXT_WORD0));
	FDEERR("CONTEXT_WORD1 %p = %x\n", CONTEXT_WORD1, FDE_READ32(CONTEXT_WORD1));
	FDEERR("CONTEXT_WORD3 %p = %x\n", CONTEXT_WORD3, FDE_READ32(CONTEXT_WORD3));

	FDEERR("REG_COM_A  %p = %x\n", REG_COM_A, FDE_READ32(REG_COM_A));
	FDEERR("REG_COM_B  %p = %x\n", REG_COM_B, FDE_READ32(REG_COM_B));
	FDEERR("REG_COM_C  %p = %x\n", REG_COM_C, FDE_READ32(REG_COM_C));

	FDEERR("REG_COM_D %p = %x\n", REG_COM_D, FDE_READ32(REG_COM_D));
	FDEERR("REG_COM_E %p = %x\n", REG_COM_E, FDE_READ32(REG_COM_E));
	FDEERR("REG_COM_F %p = %x\n", REG_COM_F, FDE_READ32(REG_COM_F));
	FDEERR("REG_COM_G %p = %x\n", REG_COM_G, FDE_READ32(REG_COM_G));
	FDEERR("REG_COM_H %p = %x\n", REG_COM_H, FDE_READ32(REG_COM_H));
	FDEERR("REG_COM_I %p = %x\n", REG_COM_I, FDE_READ32(REG_COM_I));
	FDEERR("REG_COM_J %p = %x\n", REG_COM_J, FDE_READ32(REG_COM_J));
	FDEERR("REG_COM_K %p = %x\n", REG_COM_K, FDE_READ32(REG_COM_K));

	FDEERR("REG_COM_L %p = %x\n", REG_COM_L, FDE_READ32(REG_COM_L));
	FDEERR("REG_COM_M %p = %x\n", REG_COM_M, FDE_READ32(REG_COM_M));
	FDEERR("REG_COM_N %p = %x\n", REG_COM_N, FDE_READ32(REG_COM_N));
	FDEERR("REG_COM_O %p = %x\n", REG_COM_O, FDE_READ32(REG_COM_O));
	FDEERR("REG_COM_P %p = %x\n", REG_COM_P, FDE_READ32(REG_COM_P));
	FDEERR("REG_COM_Q %p = %x\n", REG_COM_Q, FDE_READ32(REG_COM_Q));
	FDEERR("REG_COM_R %p = %x\n", REG_COM_R, FDE_READ32(REG_COM_R));
	FDEERR("REG_COM_S %p = %x\n", REG_COM_S, FDE_READ32(REG_COM_S));

	FDEERR("REG_COM_T %p = %x\n", REG_COM_T, FDE_READ32(REG_COM_T));
	FDEERR("REG_COM_U %p = %x\n", REG_COM_U, FDE_READ32(REG_COM_U));

	FDEERR(" ################################ FDE_AES DUMP END ################################\n");

	return FDE_OK;
}

static void fde_aes_test_case(u32 testcase)
{
	void __iomem *FDE_AES_CORE_BASE = fde_aes_context.base;
	u32 status = 0;

	if (!fde_aes_context.base)
		return;

	FDEERR("Test Case %d\n", testcase);
	fde_aes_context.test_case = testcase;

	switch (testcase) {

	case 0: /* Normal */
		FDEERR("Normal Test\n");
		break;
	case 1: /* Normal write/read register */
		FDEERR("Normal write/read secure only register\n");
		FDE_WRITE32(REG_COM_A, 0xffffffff);
		if (FDE_READ32(REG_COM_A))
			status |= testcase;
		FDE_WRITE32(REG_COM_B, 0xffffffff);
		if (FDE_READ32(REG_COM_B))
			status |= testcase;
		FDE_WRITE32(REG_COM_C, 0xffffffff);
		if (FDE_READ32(REG_COM_C))
			status |= testcase;
		FDE_WRITE32(REG_COM_D, 0xffffffff);
		if (FDE_READ32(REG_COM_D))
			status |= testcase;
		FDE_WRITE32(REG_COM_E, 0xffffffff);
		if (FDE_READ32(REG_COM_E))
			status |= testcase;
		FDE_WRITE32(REG_COM_F, 0xffffffff);
		if (FDE_READ32(REG_COM_F))
			status |= testcase;
		FDE_WRITE32(REG_COM_G, 0xffffffff);
		if (FDE_READ32(REG_COM_G))
			status |= testcase;
		FDE_WRITE32(REG_COM_H, 0xffffffff);
		if (FDE_READ32(REG_COM_H))
			status |= testcase;
		FDE_WRITE32(REG_COM_I, 0xffffffff);
		if (FDE_READ32(REG_COM_I))
			status |= testcase;
		FDE_WRITE32(REG_COM_J, 0xffffffff);
		if (FDE_READ32(REG_COM_J))
			status |= testcase;
		FDE_WRITE32(REG_COM_K, 0xffffffff);
		if (FDE_READ32(REG_COM_K))
			status |= testcase;
		FDE_WRITE32(REG_COM_L, 0xffffffff);
		if (FDE_READ32(REG_COM_L))
			status |= testcase;
		FDE_WRITE32(REG_COM_M, 0xffffffff);
		if (FDE_READ32(REG_COM_M))
			status |= testcase;
		FDE_WRITE32(REG_COM_N, 0xffffffff);
		if (FDE_READ32(REG_COM_N))
			status |= testcase;
		FDE_WRITE32(REG_COM_O, 0xffffffff);
		if (FDE_READ32(REG_COM_O))
			status |= testcase;
		FDE_WRITE32(REG_COM_P, 0xffffffff);
		if (FDE_READ32(REG_COM_P))
			status |= testcase;
		FDE_WRITE32(REG_COM_Q, 0xffffffff);
		if (FDE_READ32(REG_COM_Q))
			status |= testcase;
		FDE_WRITE32(REG_COM_R, 0xffffffff);
		if (FDE_READ32(REG_COM_R))
			status |= testcase;
		FDE_WRITE32(REG_COM_S, 0xffffffff);
		if (FDE_READ32(REG_COM_S))
			status |= testcase;
		FDE_WRITE32(REG_COM_T, 0xffffffff);
		if (FDE_READ32(REG_COM_T))
			status |= testcase;
		FDE_WRITE32(REG_COM_U, 0xffffffff);
		if (FDE_READ32(REG_COM_U))
			status |= testcase;
		FDEERR("Test Case-%d %s\n", testcase, status == 0 ? "PASS" : "FAIL");
		break;

	case 2: /* Write Plain-text and Read Cipher-text */
		FDEERR("Write Plain-text and Read Cipher-text\n");
		FDEERR("Please execute # emmc_rw_test 1& sd_rw_test 1& to test\n");
		break;

	case 3: /* eMMC with FDE_AES and SD without FDE_AES */
		FDEERR("eMMC with FDE_AES and SD without FDE_AES\n");
		FDEERR("Please execute # emmc_rw_test 1& sd_rw_test 1& to test\n");
		break;

	case 10: /* Dump Register */
		fde_aes_dump_reg();
		mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0, 0xa, 0); /* Dump */
		break;

	case 11: /* ByPass FDE_AES */
		FDEERR("ByPass FDE_AES\n");
		break;

	case 100: /* Enable Uart Message */
		FDEERR("Enable Uart Message\n");
		break;

	default:
		FDEERR("Err Case %d\n", testcase);
		break;
	}
}

static ssize_t fde_aes_write(struct file *file, const char __user *buffer,
				size_t count, loff_t *ppos)
{
	char buf[16];
	size_t len = count;
	int n;

	if (len >= sizeof(buf))
		len = sizeof(buf) - 1;

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	if (kstrtol(buf, 10, (long int *)&n))
		return -EINVAL;

	FDEERR("parameter %d\n", n);

	fde_aes_test_case(n);

	return len;
}

static ssize_t fde_aes_read(struct file *file, char __user *buffer,
				size_t count, loff_t *ppos)
{
	fde_aes_dump_reg();

	return 1;
}

static const struct file_operations fde_aes_proc_fops = {
	.read = fde_aes_read,
	.write = fde_aes_write,
};

static int __init fde_aes_mod_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("fde_aes", S_IRUGO | S_IWUGO, NULL, &fde_aes_proc_fops); /* Also for userload version. */

	if (!entry) {

		pr_err("[%s]: failed to create /proc/fde_aes\n", __func__);
		return -ENOMEM;
	}

	FDEERR("create /proc/fde_aes\n");

	return 0;
}
#endif

static s32 fde_aes_set_context(void)
{
	void __iomem *FDE_AES_CORE_BASE = fde_aes_context.base;

	if (!fde_aes_context.base)
		return -FDE_ENODEV;

	/* Check CONTEXT_WORD */
	if (fde_aes_context.context_id & 0xfe)
		return -FDE_EINVAL;

	/* Check CONTEXT_BP */
	if (fde_aes_context.context_bp & 0xfe)
		return -FDE_EINVAL;

	/* Check Data unit (sector) size */
	if (fde_aes_context.sector_size == 0 || fde_aes_context.sector_size > 0x4)
		return -FDE_EINVAL;

	/* Context */
	FDE_WRITE32(CONTEXT_SEL, fde_aes_context.context_id);
	FDE_WRITE32(CONTEXT_WORD0,
		((fde_aes_context.sector_size << 6) | (fde_aes_context.context_id << 1) | fde_aes_context.context_bp));
	FDE_WRITE32(CONTEXT_WORD1, fde_aes_context.sector_offset_L);
	FDE_WRITE32(CONTEXT_WORD3, fde_aes_context.sector_offset_H);

	return FDE_OK;
}

/* Check if enable by eMMC and SD module */
s32 fde_aes_check_enable(s32 dev_num, u8 bEnable)
{
	#define MSDC_MAX_ID 2 /* 0:eMMC; 1:SD; 2:SDIO */
	static int iCnt[MSDC_MAX_ID] = {0};

	if (dev_num >= 2) /* SDIO etc */
		return 0;

	spin_lock(&fde_aes_context.lock);

	FDELOG("Before check MSDC%d dev-%s use-%s status-%s\n",
		dev_num, iCnt[dev_num]?"enable":"disable",
		bEnable?"enable":"disable",
		fde_aes_context.status?"enable":"disable");

	/* Suspend */
	if (!bEnable && (iCnt[dev_num] == 1)) {
		if (iCnt[!dev_num] == 0) {
			FDELOG("SMC MSDC%d off\n", dev_num);
			mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xb, 0, 0);
			fde_aes_context.status = 0;
		}
		iCnt[dev_num] -= 1;
	}

	/* Resume */
	if (bEnable && (iCnt[dev_num] == 0)) {
		if (iCnt[!dev_num] == 0) {
			FDELOG("SMC MSDC%d on\n", dev_num);
			mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xa, 0, 0);
			fde_aes_context.status = 1;
		}
		iCnt[dev_num] += 1;
	}

	FDELOG("After check MSDC%d dev-%s use-%s status-%s\n",
		dev_num, iCnt[dev_num]?"enable":"disable",
		bEnable?"enable":"disable",
		fde_aes_context.status?"enable":"disable");

	if (iCnt[dev_num] < 0) /* WARN_ON(iCnt[dev_num] < 0); */
		iCnt[dev_num] = 0;

	spin_unlock(&fde_aes_context.lock);

	return 0;
}

s32 fde_aes_exec(s32 dev_num, u32 blkcnt, u32 opcode)
{
	s32 status;

	if (!fde_aes_context.status) /* Need to enable FDE when MSDC send command */
		FDEERR("MSDC%d block 0x%x CMD%d exec\n", dev_num, blkcnt, opcode);

	BUG_ON(!fde_aes_context.status);

	spin_lock(&fde_aes_context.lock);

	fde_aes_context.context_id = dev_num;
	fde_aes_context.context_bp = 1;
	fde_aes_context.sector_size = 1;
	fde_aes_context.sector_offset_L = blkcnt;
	fde_aes_context.sector_offset_H = 0;

	#if FDE_AES_PROC
	FDEERR("test case %d\n", fde_aes_context.test_case);
	if (fde_aes_context.test_case == 2) { /* Write Plain-text and Read Cipher-text */
		if (opcode == MMC_READ_SINGLE_BLOCK || opcode == MMC_READ_MULTIPLE_BLOCK)
			fde_aes_context.context_bp = 0;
	}

	if (fde_aes_context.test_case == 3) { /* eMMC with FDE_AES and SD without FDE_AES */
		if (dev_num == FDE_MSDC1)
			fde_aes_context.context_bp = 0;
	}
	#endif

	status = fde_aes_set_context();

	spin_unlock(&fde_aes_context.lock);

	FDELOG("MSDC%d block 0x%x CMD%d exec\n", dev_num, blkcnt, opcode);

	return status;
}

s32 fde_aes_done(s32 dev_num, u32 blkcnt, u32 opcode)
{
	FDELOG("MSDC%d block 0x%x CMD%d done\n", dev_num, blkcnt, opcode);
	return 0;
}

static int __init fde_aes_init(void)
{
	struct device_node *node = NULL;
	int ret = 0;

	memset(&fde_aes_context, 0, sizeof(fde_aes_context));

	spin_lock_init(&fde_aes_context.lock);

	mt_secure_call(MTK_SIP_KERNEL_HW_FDE_AES_INIT, 0xa, 0, 0);
	fde_aes_context.status = 1;

	/* FDE_AES Base */
	node = of_find_compatible_node(NULL, NULL, "mediatek,fde_aes_core");

	if (node) {

		fde_aes_context.base = of_iomap(node, 0);
		if (!fde_aes_context.base) {

			FDEERR("DT find compatible base failed\n");
			ret = -FDE_ENODEV;
		} else {

			FDELOG("register base %p\n", fde_aes_context.base);
			#if FDE_AES_PROC
			fde_aes_mod_init();
			#endif
		}
	} else {

		FDEERR("DT find compatible node failed\n");
		ret = -FDE_ENODEV;
	}

	FDELOG("init status %s(%d)\n", ret?"fail":"pass", ret);
	return ret;
}

static void __exit fde_aes_exit(void)
{
	FDELOG("exit\n");
}

module_init(fde_aes_init);
module_exit(fde_aes_exit);

MODULE_DESCRIPTION("MTK FDE AES driver");
MODULE_LICENSE("GPL");
