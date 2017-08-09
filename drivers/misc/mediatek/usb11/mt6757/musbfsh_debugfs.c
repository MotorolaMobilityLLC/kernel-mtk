/*
 * MUSB OTG driver debugfs support
 *
 * Copyright 2010 Nokia Corporation
 * Contact: Felipe Balbi <felipe.balbi@nokia.com>
 *
 * Copyright 2015 Mediatek Inc.
 *	Marvin Lin <marvin.lin@mediatek.com>
 *	Arvin Wang <arvin.wang@mediatek.com>
 *	Vincent Fan <vincent.fan@mediatek.com>
 *	Bryant Lu <bryant.lu@mediatek.com>
 *	Yu-Chang Wang <yu-chang.wang@mediatek.com>
 *	Macpaul Lin <macpaul.lin@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>

#include "musbfsh_core.h"
#include "musbfsh_host.h"

#ifdef CONFIG_MUSBFSH_DEBUG_FS
#include <linux/proc_fs.h>
#include "usb.h"

struct musbfsh_register_map {
	char *name;
	unsigned offset;
	unsigned size;
};

static const struct musbfsh_register_map musbfsh_regmap[] = {
	{"FAddr", 0x00, 8},
	{"Power", 0x01, 8},
	{"Frame", 0x0c, 16},
	{"Index", 0x0e, 8},
	{"Testmode", 0x0f, 8},
	{"TxMaxPp", 0x10, 16},
	{"TxCSRp", 0x12, 16},
	{"RxMaxPp", 0x14, 16},
	{"RxCSR", 0x16, 16},
	{"RxCount", 0x18, 16},
	{"ConfigData", 0x1f, 8},
	{"DevCtl", 0x60, 8},
	{"MISC", 0x61, 8},
	{"TxFIFOsz", 0x62, 8},
	{"RxFIFOsz", 0x63, 8},
	{"TxFIFOadd", 0x64, 16},
	{"RxFIFOadd", 0x66, 16},
	{"VControl", 0x68, 32},
	{"HWVers", 0x6C, 16},
	{"EPInfo", 0x78, 8},
	{"RAMInfo", 0x79, 8},
	{"LinkInfo", 0x7A, 8},
	{"VPLen", 0x7B, 8},
	{"HS_EOF1", 0x7C, 8},
	{"FS_EOF1", 0x7D, 8},
	{"LS_EOF1", 0x7E, 8},
	{"SOFT_RST", 0x7F, 8},
	{"DMA_CNTLch0", 0x204, 16},
	{"DMA_ADDRch0", 0x208, 32},
	{"DMA_COUNTch0", 0x20C, 32},
	{"DMA_CNTLch1", 0x214, 16},
	{"DMA_ADDRch1", 0x218, 32},
	{"DMA_COUNTch1", 0x21C, 32},
	{"DMA_CNTLch2", 0x224, 16},
	{"DMA_ADDRch2", 0x228, 32},
	{"DMA_COUNTch2", 0x22C, 32},
	{"DMA_CNTLch3", 0x234, 16},
	{"DMA_ADDRch3", 0x238, 32},
	{"DMA_COUNTch3", 0x23C, 32},
	{"DMA_CNTLch4", 0x244, 16},
	{"DMA_ADDRch4", 0x248, 32},
	{"DMA_COUNTch4", 0x24C, 32},
	{"DMA_CNTLch5", 0x254, 16},
	{"DMA_ADDRch5", 0x258, 32},
	{"DMA_COUNTch5", 0x25C, 32},
	{"DMA_CNTLch6", 0x264, 16},
	{"DMA_ADDRch6", 0x268, 32},
	{"DMA_COUNTch6", 0x26C, 32},
	{"DMA_CNTLch7", 0x274, 16},
	{"DMA_ADDRch7", 0x278, 32},
	{"DMA_COUNTch7", 0x27C, 32},
	{"DBG_PRB0", 0x620, 32},
	{}			/* Terminating Entry */
};

static const struct musbfsh_register_map musbfsh_phyregmap[] = {
	{"USBPHYACR0", 0x00, 32},
	{"USBPHYACR1", 0x04, 32},
	{"USBPHYACR2", 0x08, 32},
	{"USBPHYACR4", 0x10, 32},
	{"USBPHYACR5", 0x14, 32},
	{"USBPHYACR6", 0x18, 32},
	{"U2PHYACR3", 0x1c, 32},
	{"U2PHYACR4", 0x20, 32},
	{"U2PHYAMON0", 0x24, 32},
	{"U2PHYDCR0", 0x60, 32},
	{"U2PHYDCR1", 0x64, 32},
	{"U2PHYDTM0", 0x68, 32},
	{"U2PHYDTM1", 0x6c, 32},
	{"U2PHYDMON0", 0x70, 32},
	{"U2PHYDMON1", 0x74, 32},
	{"U2PHYDMON2", 0x78, 32},
	{"U2PHYDMON3", 0x7c, 32},
	{"REGFCOM", 0xfc, 32},
	{}			/* Terminating Entry */
};

static struct dentry *musbfsh_debugfs_root;

static int musbfsh_regdump_show(struct seq_file *s, void *unused)
{
	struct musbfsh *musbfsh = s->private;
	unsigned i;

	seq_puts(s, "MUSBFSH (M)HDRC Register Dump\n");

	for (i = 0; i < ARRAY_SIZE(musbfsh_regmap); i++) {
		switch (musbfsh_regmap[i].size) {
		case 8:
			seq_printf(s, "%-12s: %02x\n", musbfsh_regmap[i].name,
				   musbfsh_readb(musbfsh->mregs,
				   musbfsh_regmap[i].offset));
			break;
		case 16:
			seq_printf(s, "%-12s: %04x\n", musbfsh_regmap[i].name,
				   musbfsh_readw(musbfsh->mregs,
				   musbfsh_regmap[i].offset));
			break;
		case 32:
			seq_printf(s, "%-12s: %08x\n", musbfsh_regmap[i].name,
				   musbfsh_readl(musbfsh->mregs,
				   musbfsh_regmap[i].offset));
			break;
		}
	}

	return 0;
}


static int musbfsh_phyregdump_show(struct seq_file *s, void *unused)
{
	struct musbfsh *musbfsh = s->private;
	unsigned i;

	seq_puts(s, "MUSBFSH (M)HDRC PhyRegister Dump\n");

	for (i = 0; i < ARRAY_SIZE(musbfsh_phyregmap); i++) {
		switch (musbfsh_phyregmap[i].size) {
		case 8:
			seq_printf(s, "%-12s: %02x\n",
				   musbfsh_phyregmap[i].name,
				   musbfsh_readb(musbfsh->phy_base + 0x900,
						 musbfsh_phyregmap[i].offset));
			break;
		case 16:
			seq_printf(s, "%-12s: %04x\n",
				   musbfsh_phyregmap[i].name,
				   musbfsh_readw(musbfsh->phy_base + 0x900,
						 musbfsh_phyregmap[i].offset));
			break;
		case 32:
			seq_printf(s, "%-12s: %08x\n",
				   musbfsh_phyregmap[i].name,
				   musbfsh_readl(musbfsh->phy_base + 0x900,
						 musbfsh_phyregmap[i].offset));
			break;
		}
	}

	return 0;
}


static int musbfsh_regdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, musbfsh_regdump_show, inode->i_private);
}

static int musbfsh_phyregdump_open(struct inode *inode, struct file *file)
{
	return single_open(file, musbfsh_phyregdump_show, inode->i_private);
}

static int musbfsh_test_mode_show(struct seq_file *s, void *unused)
{
	struct musbfsh *musbfsh = s->private;
	unsigned test;

	test = musbfsh_readb(musbfsh->mregs, MUSBFSH_TESTMODE);

	if (test & MUSBFSH_TEST_FORCE_HOST)
		seq_puts(s, "force host\n");

	if (test & MUSBFSH_TEST_FIFO_ACCESS)
		seq_puts(s, "fifo access\n");

	if (test & MUSBFSH_TEST_FORCE_FS)
		seq_puts(s, "force full-speed\n");

	if (test & MUSBFSH_TEST_FORCE_HS)
		seq_puts(s, "force high-speed\n");

	if (test & MUSBFSH_TEST_PACKET)
		seq_puts(s, "test packet\n");

	if (test & MUSBFSH_TEST_K)
		seq_puts(s, "test K\n");

	if (test & MUSBFSH_TEST_J)
		seq_puts(s, "test J\n");

	if (test & MUSBFSH_TEST_SE0_NAK)
		seq_puts(s, "test SE0 NAK\n");

	return 0;
}

static const struct file_operations musbfsh_regdump_fops = {
	.open = musbfsh_regdump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations musbfsh_phyregdump_fops = {
	.open = musbfsh_phyregdump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int musbfsh_test_mode_open(struct inode *inode, struct file *file)
{
	return single_open(file, musbfsh_test_mode_show, inode->i_private);
}

static ssize_t musbfsh_test_mode_write(struct file *file,
				       const char __user *ubuf,
				       size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct musbfsh *musbfsh = s->private;
	u8 test = 0;
	char buf[18];

	memset(buf, 0x00, sizeof(buf));

	if (copy_from_user(&buf, ubuf, min_t(size_t, sizeof(buf) - 1, count)))
		return -EFAULT;

	if (!strncmp(buf, "force host", 9))
		test = MUSBFSH_TEST_FORCE_HOST;

	if (!strncmp(buf, "fifo access", 11))
		test = MUSBFSH_TEST_FIFO_ACCESS;

	if (!strncmp(buf, "force full-speed", 15))
		test = MUSBFSH_TEST_FORCE_FS;

	if (!strncmp(buf, "force high-speed", 15))
		test = MUSBFSH_TEST_FORCE_HS;

	if (!strncmp(buf, "test packet", 10)) {
		test = MUSBFSH_TEST_PACKET;
		musbfsh_load_testpacket(musbfsh);
	}

	if (!strncmp(buf, "test K", 6))
		test = MUSBFSH_TEST_K;

	if (!strncmp(buf, "test J", 6))
		test = MUSBFSH_TEST_J;

	if (!strncmp(buf, "test SE0 NAK", 12))
		test = MUSBFSH_TEST_SE0_NAK;

	musbfsh_writeb(musbfsh->mregs, MUSBFSH_TESTMODE, test);

	return count;
}


ssize_t musbfsh_musbfsh_cmd_proc_entry(struct file *file_ptr,
				       const char __user *user_buffer,
				       size_t count, loff_t *position)
{
	char cmd;

	if (copy_from_user(&cmd, user_buffer, 1))
		return 0;

	switch (cmd) {
#ifdef CONFIG_MTK_ICUSB_SUPPORT
	case 'k':		/* enable vsim power */
		musbfsh_open_vsim_power(1);
		musbfsh_init_phy_by_voltage(VOL_33);
		musbfsh_start_session();
		break;
	case 'p':		/* enable vsim power */
		musbfsh_open_vsim_power1(1);
		musbfsh_init_phy_by_voltage(VOL_33);
		musbfsh_start_session();
		break;
#endif
	default:
		MYDBG("Command unsupported.\n");
		break;
	}

	return count;
}


static const struct file_operations musbfsh_ic_usb_cmd_proc_fops = {
	.write = musbfsh_musbfsh_cmd_proc_entry
};

void create_musbfsh_cmd_proc_entry(void)
{
	struct proc_dir_entry *prEntry;

	MYDBG("");
	prEntry = proc_create("MUSBFSH_CMD_ENTRY", 0660, 0,
			      &musbfsh_ic_usb_cmd_proc_fops);
	if (prEntry)
		MYDBG("add /proc/MUSBFSH_CMD_ENTRY ok\n");
	else
		MYDBG("add /proc/MUSBFSH_CMD_ENTRY fail\n");
}


static const struct file_operations musbfsh_test_mode_fops = {
	.open = musbfsh_test_mode_open,
	.write = musbfsh_test_mode_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int musbfsh_init_debugfs(struct musbfsh *musbfsh)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	root = debugfs_create_dir("musbfsh", NULL);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file("regdump", S_IRUGO, root, musbfsh,
				   &musbfsh_regdump_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("regdumpphy", S_IRUGO, root, musbfsh,
				   &musbfsh_phyregdump_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("testmode", S_IRUGO | S_IWUSR,
				   root, musbfsh, &musbfsh_test_mode_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	musbfsh_debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void /* __init_or_exit */ musbfsh_exit_debugfs(struct musbfsh *musbfsh)
{
	debugfs_remove_recursive(musbfsh_debugfs_root);
}

#endif
