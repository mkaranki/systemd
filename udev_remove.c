/*
 * udev-remove.c
 *
 * Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 * 
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "udev.h"

static int delete_node(struct udevice *udev)
{
	char filename[PATH_SIZE];
	char devname[PATH_SIZE];
	char partitionname[PATH_SIZE];
	struct name_entry *name_loop;
	struct stat stats;
	int retval;
	int i;
	int num;

	list_for_each_entry(name_loop, &udev->symlink_list, node) {
		snprintf(filename, sizeof(filename), "%s/%s", udev_root, name_loop->name);
		filename[sizeof(filename)-1] = '\0';

		if (stat(filename, &stats) != 0) {
			dbg("symlink '%s' not found", filename);
			continue;
		}
		if (udev->devt && stats.st_rdev != udev->devt) {
			info("symlink '%s' points to a different device, skip removal", filename);
			continue;;
		}

		info("removing symlink '%s'", filename);
		unlink(filename);

		if (strchr(filename, '/'))
			delete_path(filename);
	}

	snprintf(filename, sizeof(filename), "%s/%s", udev_root, udev->name);
	filename[sizeof(filename)-1] = '\0';

	if (stat(filename, &stats) != 0) {
		dbg("device node '%s' not found", filename);
		return -1;
	}
	if (udev->devt && stats.st_rdev != udev->devt) {
		info("device node '%s' points to a different device, skip removal", filename);
		return -1;
	}

	info("removing device node '%s'", filename);
	retval = unlink_secure(filename);
	if (retval)
		return retval;

	snprintf(devname, sizeof(devname), "%s/%s", udev_root, udev->name);
	devname[sizeof(devname)-1] = '\0';
	setenv("DEVNAME", devname, 1);

	num = udev->partitions;
	if (num > 0) {
		info("removing all_partitions '%s[1-%i]'", filename, num);
		if (num > 255) {
			info("garbage from udev database, skip all_partitions removal");
			return -1;
		}
		for (i = 1; i <= num; i++) {
			snprintf(partitionname, sizeof(partitionname), "%s%d", filename, i);
			partitionname[sizeof(partitionname)-1] = '\0';
			unlink_secure(partitionname);
		}
	}

	if (strchr(udev->name, '/'))
		delete_path(filename);

	return retval;
}

/*
 * look up the sysfs path in the database to get the node name to remove
 * If we can't find it, use kernel name for lack of anything else to know to do
 */
int udev_remove_device(struct udevice *udev)
{
	if (major(udev->devt) == 0)
		return 0;

	if (udev_db_get_device(udev, udev->dev->devpath) == 0) {
		if (udev->ignore_remove) {
			dbg("remove event for '%s' requested to be ignored by rule", udev->name);
			return 0;
		}
		dbg("remove name='%s'", udev->name);
		udev_db_delete_device(udev);
	} else {
		dbg("'%s' not found in database, using kernel name '%s'", udev->dev->devpath, udev->dev->kernel_name);
		strlcpy(udev->name, udev->dev->kernel_name, sizeof(udev->name));
	}

	return delete_node(udev);
}
