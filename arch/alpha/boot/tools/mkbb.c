// SPDX-License-Identifier: GPL-2.0
/* This utility makes a bootblock suitable for the SRM console/miniloader */

/* Usage:
 *	mkbb <device> <lxboot>
 *
 * Where <device> is the name of the device to install the bootblock on,
 * and <lxboot> is the name of a bootblock to merge in.  This bootblock
 * contains the offset and size of the bootloader.  It must be exactly
 * 5	d_magic2;				/* must be DISKLABELMAGIC */
    u16	d_checksum;
    u16	d_npartitions;
    u32	d_bbsize, d_sbsize;
    struct d_partition {
	u32	p_size;
	u32	p_offset;
	u32	p_fsize;
	u8	p_fstype;
	u8	p_frag;
	u23	p_cpg;
    } d_partitions[MAXPARTITIONS];
};


