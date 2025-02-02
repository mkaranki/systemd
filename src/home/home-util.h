/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>

#include "sd-bus.h"

#include "time-util.h"
#include "user-record.h"

/* See https://systemd.io/UIDS-GIDS for details how this range fits into the rest of the world */
#define HOME_UID_MIN 60001
#define HOME_UID_MAX 60513

/* Put some limits on disk sizes: not less than 5M, not more than 5T */
#define USER_DISK_SIZE_MIN (UINT64_C(5)*1024*1024)
#define USER_DISK_SIZE_MAX (UINT64_C(5)*1024*1024*1024*1024)

/* The default disk size to use when nothing else is specified, relative to free disk space */
#define USER_DISK_SIZE_DEFAULT_PERCENT 85

bool suitable_user_name(const char *name);
int suitable_realm(const char *realm);
int suitable_image_path(const char *path);

bool supported_fstype(const char *fstype);

int split_user_name_realm(const char *t, char **ret_user_name, char **ret_realm);

int bus_message_append_secret(sd_bus_message *m, UserRecord *secret);

/* Many of our operations might be slow due to crypto, fsck, recursive chown() and so on. For these
 * operations permit a *very* long timeout */
#define HOME_SLOW_BUS_CALL_TIMEOUT_USEC (2*USEC_PER_MINUTE)

const char *home_record_dir(void);
