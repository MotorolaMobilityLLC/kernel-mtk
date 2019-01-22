/* Mediatek STAR MAC network driver.
 *
 * Copyright (c) 2016-2017 Mediatek Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include "star.h"
#include "star_procfs.h"

static struct star_procfs star_proc;

static bool str_cmp_seq(char **buf, const char *substr)
{
	size_t len = strlen(substr);

	if (!strncmp(*buf, substr, len)) {
		*buf += len + 1;
		return true;
	} else {
		return false;
	}
}

static struct net_device *star_get_net_device(void)
{
	if (!star_proc.ndev)
		star_proc.ndev = dev_get_by_name(&init_net, "eth0");

	return star_proc.ndev;
}

static void star_put_net_device(void)
{
	if (!star_proc.ndev)
		return;

	dev_put(star_proc.ndev);
}

static ssize_t proc_phy_reg_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	STAR_MSG(STAR_ERR, "read phy register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo rp reg_addr > phy_reg\n");

	STAR_MSG(STAR_ERR, "write phy register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo wp reg_addr value > phy_reg\n");

	return 0;
}

static ssize_t proc_reg_write(struct file *file,
			      const char __user *buffer,
			      size_t count, loff_t *pos)
{
	char *buf, *tmp;
	u16 phy_val;
	u32 i, mac_val, len = 0, address = 0, value = 0;
	struct net_device *dev;
	star_private *star_prv;
	star_dev *star_dev;

	tmp = kmalloc(count + 1, GFP_KERNEL);
	buf = tmp;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	dev = star_get_net_device();
	if (!dev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	star_prv = netdev_priv(dev);
	star_dev = &star_prv->star_dev;

	if (str_cmp_seq(&buf, "rp")) {
		if (!kstrtou32(buf, 0, &address)) {
			STAR_MSG(STAR_ERR, "address(0x%x):0x%x\n",
				 address,
				 star_mdc_mdio_read(star_dev,
						    star_prv->phy_addr,
						    address));
		} else {
			STAR_MSG(STAR_ERR, "kstrtou32 rp(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "wp")) {
		if (sscanf(buf, "%x %x", &address, &value) == 2) {
			phy_val = star_mdc_mdio_read(star_dev,
						     star_prv->phy_addr,
						     address);
			star_mdc_mdio_write(star_dev, star_prv->phy_addr,
					    address, (u16)value);
			STAR_MSG(STAR_ERR, "0x%x: 0x%x --> 0x%x!\n",
				 address, phy_val,
				 star_mdc_mdio_read(star_dev,
						    star_prv->phy_addr,
						    address));
		} else {
			STAR_MSG(STAR_ERR, "sscanf wp(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "rr")) {
		if (sscanf(buf, "%x %x", &address, &len) == 2) {
			for (i = 0; i < len / 4; i++) {
				STAR_MSG(STAR_ERR,
					 "%p:\t%08x\t%08x\t%08x\t%08x\t\n",
					 star_dev->base + address + i * 16,
					 star_get_reg(star_dev->base
						    + address + i * 16),
					 star_get_reg(star_dev->base + address
						    + i * 16 + 4),
					 star_get_reg(star_dev->base + address
						    + i * 16 + 8),
					 star_get_reg(star_dev->base + address
						    + i * 16 + 12));
			}
		} else {
			STAR_MSG(STAR_ERR, "sscanf rr(%s) error\n", buf);
		}
	} else if (str_cmp_seq(&buf, "wr")) {
		if (sscanf(buf, "%x %x", &address, &value) == 2) {
			mac_val = star_get_reg(star_dev->base + address);
			star_set_reg(star_dev->base + address, value);
			STAR_MSG(STAR_ERR, "%p: %08x --> %08x!\n",
				 star_dev->base + address,
				 mac_val,
				 star_get_reg(star_dev->base
					    + address));
		} else {
			STAR_MSG(STAR_ERR, "sscanf wr(%s) error\n", buf);
		}
	} else {
		STAR_MSG(STAR_ERR, "wrong arg:%s\n", buf);
	}

	kfree(tmp);
	return count;
}

static const struct file_operations star_phy_reg_ops = {
	.read = proc_phy_reg_read,
	.write = proc_reg_write,
};

static ssize_t proc_mac_reg_read(struct file *file, char __user *buf,
				 size_t count, loff_t *ppos)
{
	STAR_MSG(STAR_ERR, "read MAC register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo rr reg_addr len > macreg\n");

	STAR_MSG(STAR_ERR, "write MAC register useage:\n");
	STAR_MSG(STAR_ERR, "\t echo wr reg_addr value > macreg\n");

	return 0;
}

static const struct file_operations star_mac_reg_ops = {
	.read = proc_mac_reg_read,
	.write = proc_reg_write,
};

static ssize_t proc_read_wol_eth(struct file *file,
				 char __user *buf, size_t count, loff_t *ppos)
{
	int wol;
	struct net_device *ndev;
	star_private *star_prv;

	ndev = star_get_net_device();
	if (!ndev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	star_prv = netdev_priv(ndev);
	wol = star_get_wol_flag(star_prv);
	if (wol < 0)
		return -1;

	STAR_MSG(STAR_ERR, "%ssupport WOL\n", (wol ? "" : "NOT "));
	STAR_MSG(STAR_ERR,
		 "Use 'echo 1 > /proc/driver/star/wol' to enable wol\n");
	STAR_MSG(STAR_ERR,
		 "Use 'echo 0 > /proc/driver/star/wol' to disable wol\n");

	return 0;
}

static ssize_t proc_write_wol_eth(struct file *file, const char __user *buffer,
				  size_t count, loff_t *pos)
{
	char *buf;
	struct net_device *ndev;
	star_private *star_prv;

	ndev = star_get_net_device();
	if (!ndev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	star_prv = netdev_priv(ndev);
	buf = kmalloc(count + 1, GFP_KERNEL);
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;

	buf[count] = '\0';
	star_set_wol_flag(star_prv, (buf[0] - '0'));

	kfree(buf);

	return count;
}

static const struct file_operations star_wol_ops = {
	.read = proc_read_wol_eth,
	.write = proc_write_wol_eth,
};

static ssize_t proc_dump_net_stat(struct file *file,
				  char __user *buf, size_t count, loff_t *ppos)
{
	struct net_device *ndev;
	star_private *star_prv;
	star_dev *star_dev;

	ndev = star_get_net_device();
	if (!ndev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	star_prv = netdev_priv(ndev);
	star_dev = &star_prv->star_dev;

	STAR_MSG(STAR_ERR, "\n");
	STAR_MSG(STAR_ERR, "rx_packets	=%lu  <total packets received>\n",
		 star_dev->stats.rx_packets);
	STAR_MSG(STAR_ERR, "tx_packets	=%lu  <total packets transmitted>\n",
		 star_dev->stats.tx_packets);
	STAR_MSG(STAR_ERR, "rx_bytes	=%lu  <total bytes received>\n",
		 star_dev->stats.rx_bytes);
	STAR_MSG(STAR_ERR, "tx_bytes	=%lu  <total bytes transmitted>\n",
		 star_dev->stats.tx_bytes);
	STAR_MSG(STAR_ERR, "rx_errors;	=%lu  <bad packets received>\n",
		 star_dev->stats.rx_errors);
	STAR_MSG(STAR_ERR, "tx_errors;	=%lu  <packet transmit problems>\n",
		 star_dev->stats.tx_errors);
	STAR_MSG(STAR_ERR, "rx_crc_errors =%lu  <recved pkt with crc error>\n",
		 star_dev->stats.rx_crc_errors);
	STAR_MSG(STAR_ERR, "\n");
	STAR_MSG(STAR_ERR,
		 "Use 'cat /proc/driver/star/stat' to dump net info\n");
	STAR_MSG(STAR_ERR,
		 "Use 'echo clear > /proc/driver/star/stat' to clear info\n");

	return 0;
}

static ssize_t proc_clear_net_stat(struct file *file,
				   const char __user *buffer,
				   size_t count, loff_t *pos)
{
	char *buf;
	struct net_device *ndev;
	star_private *star_prv;
	star_dev *star_dev;

	buf = kmalloc(count + 1, GFP_KERNEL);
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	ndev = star_get_net_device();
	if (!ndev) {
		STAR_MSG(STAR_ERR, "Could not get eth0 device!!!\n");
		return -1;
	}

	star_prv = netdev_priv(ndev);
	star_dev = &star_prv->star_dev;

	if (!strncmp(buf, "clear", count - 1))
		memset(&star_dev->stats, 0, sizeof(struct net_device_stats));
	else
		STAR_MSG(STAR_ERR, "fail to clear stat, buf:%s\n", buf);

	kfree(buf);

	return count;
}

static const struct file_operations star_net_status_ops = {
	.read = proc_dump_net_stat,
	.write = proc_clear_net_stat,
};

static struct star_proc_file star_file_tbl[] = {
	{"phy_reg", &star_phy_reg_ops},
	{"macreg", &star_mac_reg_ops},
	{"wol", &star_wol_ops},
	{"stat", &star_net_status_ops},
};

int star_init_procfs(void)
{
	int i;

	STAR_MSG(STAR_ERR, "%s entered\n", __func__);
	star_proc.root = proc_mkdir("driver/star", NULL);
	if (!star_proc.root) {
		STAR_MSG(STAR_ERR, "star_proc_dir create failed\n");
		return -1;
	}

	star_proc.entry = kmalloc(ARRAY_SIZE(star_file_tbl) *
				  sizeof(struct star_proc_file), GFP_KERNEL);
	for (i = 0 ; i < ARRAY_SIZE(star_file_tbl); i++) {
		star_proc.entry[i] = proc_create(star_file_tbl[i].name,
			0755, star_proc.root, star_file_tbl[i].fops);
		if (!star_proc.entry[i]) {
			STAR_MSG(STAR_ERR,
				 "%s create failed\n", star_file_tbl[i].name);
			return -1;
		}
	}

	return 0;
}

void star_exit_procfs(void)
{
	int i;

	STAR_MSG(STAR_ERR, "%s entered\n", __func__);
	for (i = 0 ; i < ARRAY_SIZE(star_file_tbl); i++)
		remove_proc_entry(star_file_tbl[i].name, star_proc.root);

	kfree(star_proc.entry);
	remove_proc_entry("driver/star", NULL);
	star_put_net_device();
}

