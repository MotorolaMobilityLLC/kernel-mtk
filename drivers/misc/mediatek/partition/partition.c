#include <linux/types.h>
#include <linux/genhd.h>

struct hd_struct *get_part(char *name)
{
	dev_t devt;
	int partno;
	struct disk_part_iter piter;
	struct gendisk *disk;
	struct hd_struct *part = NULL;

	if (!name)
		return part;

	devt = blk_lookup_devt("mmcblk0", 0);
	disk = get_gendisk(devt, &partno);

	if (!disk || get_capacity(disk) == 0)
		return 0;

	disk_part_iter_init(&piter, disk, 0);
	while ((part = disk_part_iter_next(&piter))) {
		if (part->info && !strcmp(part->info->volname, name)) {
			get_device(part_to_dev(part));
			break;
		}
	}
	disk_part_iter_exit(&piter);

	return part;
}
EXPORT_SYMBOL(get_part);

void put_part(struct hd_struct *part)
{
	disk_put_part(part);
}
EXPORT_SYMBOL(put_part);
