/* SPDX-License-Identifier: GPL-2.0 */
/*
 * vsensor_ioctl.h - shared kernel/user ABI for the virtual sensor driver.
 * Included by driver/vsensor.c AND by the C++ user-space code.
 */
#ifndef VSENSOR_IOCTL_H
#define VSENSOR_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define VSENSOR_DEV_NAME "vsensor"

/* Fault modes the driver can inject (values must match sg::InjectMode). */
enum vsensor_fault_mode {
	VS_FAULT_NONE = 0,
	VS_FAULT_STUCK,		/* output frozen at last good value   */
	VS_FAULT_SPIKE,		/* periodic large outliers            */
	VS_FAULT_DRIFT,		/* slow, growing offset               */
	VS_FAULT_NOISE,		/* greatly increased noise amplitude  */
	VS_FAULT_DROPOUT,	/* sample flagged invalid, value = 0  */
	VS_FAULT_MAX
};

#define VS_FLAG_VALID 0x1u

/* One reading returned by read(). Temperature is in milli-degrees Celsius. */
struct vsensor_sample {
	__u64 timestamp_ns;
	__s32 value_mC;
	__u32 seq;
	__u32 flags;
	__u32 reserved;
};

#define VSENSOR_IOC_MAGIC 'v'
#define VSENSOR_IOC_SET_FAULT _IOW(VSENSOR_IOC_MAGIC, 1, __u32)
#define VSENSOR_IOC_GET_FAULT _IOR(VSENSOR_IOC_MAGIC, 2, __u32)
#define VSENSOR_IOC_RESET     _IO(VSENSOR_IOC_MAGIC, 3)

#endif /* VSENSOR_IOCTL_H */
