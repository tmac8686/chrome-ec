/* Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * EC-EFS (Early Firmware Selection)
 */
#include "common.h"
#include "console.h"
#include "ec_comm.h"
#include "crc8.h"
#include "ec_commands.h"
#include "hooks.h"
#include "registers.h"
#include "system.h"
#include "tpm_nvmem.h"
#include "tpm_nvmem_ops.h"
#include "vboot.h"

#ifdef CR50_DEV
#define CPRINTS(format, args...) cprints(CC_TASK, "EC-EFS: " format, ## args)
#else
#define CPRINTS(format, args...) do { } while (0)
#endif

/*
 * Context of EC-EFS
 */
static struct ec_efs_context_ {
	uint32_t boot_mode:8;	        /* enum ec_efs_boot_mode */
	uint32_t hash_is_loaded:1;	/* Is EC hash loaded from nvmem */
	uint32_t reserved:23;

	uint32_t secdata_error_code;

	uint8_t hash[SHA256_DIGEST_SIZE];	/* EC-RW digest */
} ec_efs_ctx;

/*
 * Change the boot mode
 *
 * @param mode_val New boot mode value to change
 */
static void set_boot_mode_(uint8_t mode_val)
{
	CPRINTS("boot_mode: 0x%02x -> 0x%02x", ec_efs_ctx.boot_mode, mode_val);

	ec_efs_ctx.boot_mode = mode_val;

	/* Backup some ec_efs context to scratch register */
	GREG32(PMU, PWRDN_SCRATCH20) &= ~0xff;
	GREG32(PMU, PWRDN_SCRATCH20) |= mode_val;
}

static int load_ec_hash_(uint8_t * const ec_hash)
{
	struct vb2_secdata_kernel secdata;
	const uint8_t secdata_size = sizeof(struct vb2_secdata_kernel);
	uint8_t size_to_crc;
	uint8_t struct_size;
	uint8_t crc;

	if (read_tpm_nvmem(KERNEL_NV_INDEX, secdata_size,
			   (void *)&secdata) != tpm_read_success)
		return EC_ERROR_VBOOT_DATA_UNDERSIZED;

	/*
	 * Check struct version. CRC offset may be different with old struct
	 * version
	 */
	if (secdata.struct_version < VB2_SECDATA_KERNEL_STRUCT_VERSION_MIN)
		return EC_ERROR_VBOOT_DATA_INCOMPATIBLE;

	/* Check struct size. */
	struct_size = secdata.struct_size;
	if (struct_size != secdata_size)
		return EC_ERROR_VBOOT_DATA;

	/* Check CRC */
	size_to_crc = struct_size -
		      offsetof(struct vb2_secdata_kernel, crc8) -
		      sizeof(secdata.crc8);
	crc = crc8((uint8_t *)&secdata.reserved0, size_to_crc);
	if (crc != secdata.crc8)
		return EC_ERROR_CRC;

	/* Read hash and copy to hash */
	memcpy(ec_hash, secdata.ec_hash, sizeof(secdata.ec_hash));

	return EC_SUCCESS;
}

/*
 * Initialize EC-EFS context.
 */
static void ec_efs_init_(void)
{
	if (!board_has_ec_cr50_comm_support())
		return;

	/*
	 * If it is a wakeup from deep sleep, then recover some core EC-EFS
	 * context values, including the boot_mode value, from a PWRD_SCRATCH
	 * register. Otherwise, reset boot_mode.
	 */
	if (system_get_reset_flags() & EC_RESET_FLAG_HIBERNATE)
		set_boot_mode_(GREG32(PMU, PWRDN_SCRATCH20) & 0xff);
	else
		ec_efs_reset();

	/* Read an EC hash in kernel secdata (TPM kernel NV index). */
	if (ec_efs_ctx.hash_is_loaded)
		return;

	ec_efs_refresh();
}
DECLARE_HOOK(HOOK_INIT, ec_efs_init_, HOOK_PRIO_DEFAULT);

void ec_efs_reset(void)
{
	set_boot_mode_(EC_EFS_BOOT_MODE_NORMAL);
}

void ec_efs_refresh(void)
{
	int rv;

	rv = load_ec_hash_(ec_efs_ctx.hash);
	if (rv == EC_SUCCESS) {
		ec_efs_ctx.hash_is_loaded = 1;
	} else {
		ec_efs_ctx.hash_is_loaded = 0;
		cprints(CC_SYSTEM, "load_ec_hash error: 0x%x\n", rv);
	}
	ec_efs_ctx.secdata_error_code = rv;
}
