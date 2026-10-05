// SPDX-License-Identifier: GPL-2.0
/*
 * vsensor.c - Virtual temperature sensor with injectable faults.
 *
 * Registers a misc character device /dev/vsensor.
 *   read()  -> returns one struct vsensor_sample per call
 *   ioctl() -> select / query fault mode, reset the sensor
 *
 * The data generator is deterministic (LCG) so tests are repeatable.
 * It deliberately mirrors src/simulated_source.cpp in user space.
 */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ktime.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include "vsensor_ioctl.h"

#define VS_NOISE_NORMAL_MC   200   /* +-0.2 C healthy noise   */
#define VS_NOISE_FAULT_MC   4000   /* +-4.0 C noise fault     */
#define VS_SPIKE_MC        25000   /* +25 C spike             */
#define VS_SPIKE_PERIOD        3   /* every 3rd sample        */
#define VS_DRIFT_STEP_MC     100   /* +0.1 C per sample       */

static int base_temp_mc = 25000;
module_param(base_temp_mc, int, 0444);
MODULE_PARM_DESC(base_temp_mc, "Healthy base temperature in milli-degC");

static struct {
	struct mutex lock;
	u32 seq;
	u32 fault;
	u32 fault_seq;		/* seq at which the fault was injected */
	s32 last_good_mc;
	u32 rng;
} vs;

static u32 vs_rand(void)
{
	vs.rng = vs.rng * 1664525u + 1013904223u;
	return vs.rng >> 16;
}

static s32 vs_noise(s32 amp)
{
	return (s32)(vs_rand() % (2 * amp + 1)) - amp;
}

static void vs_reset_locked(void)
{
	vs.seq = 0;
	vs.fault = VS_FAULT_NONE;
	vs.fault_seq = 0;
	vs.last_good_mc = base_temp_mc;
	vs.rng = 12345;
}

/* Caller holds vs.lock. */
static void vs_generate(struct vsensor_sample *s)
{
	s32 normal = base_temp_mc + vs_noise(VS_NOISE_NORMAL_MC);
	u32 since = vs.seq - vs.fault_seq;
	s32 value = normal;
	u32 flags = VS_FLAG_VALID;

	switch (vs.fault) {
	case VS_FAULT_NONE:
		vs.last_good_mc = normal;
		break;
	case VS_FAULT_STUCK:
		value = vs.last_good_mc;
		break;
	case VS_FAULT_SPIKE:
		if (since % VS_SPIKE_PERIOD == VS_SPIKE_PERIOD - 1)
			value = normal + VS_SPIKE_MC;
		break;
	case VS_FAULT_DRIFT:
		value = normal + (s32)(since * VS_DRIFT_STEP_MC);
		break;
	case VS_FAULT_NOISE:
		value = base_temp_mc + vs_noise(VS_NOISE_FAULT_MC);
		break;
	case VS_FAULT_DROPOUT:
		value = 0;
		flags = 0;
		break;
	}

	memset(s, 0, sizeof(*s));
	s->timestamp_ns = ktime_get_ns();
	s->value_mC = value;
	s->seq = vs.seq++;
	s->flags = flags;
}

static int vs_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static ssize_t vs_read(struct file *file, char __user *buf, size_t len,
		       loff_t *off)
{
	struct vsensor_sample s;

	if (len < sizeof(s))
		return -EINVAL;

	mutex_lock(&vs.lock);
	vs_generate(&s);
	mutex_unlock(&vs.lock);

	if (copy_to_user(buf, &s, sizeof(s)))
		return -EFAULT;
	return sizeof(s);
}

static long vs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	u32 val;
	long ret = 0;

	switch (cmd) {
	case VSENSOR_IOC_SET_FAULT:
		if (copy_from_user(&val, (u32 __user *)arg, sizeof(val)))
			return -EFAULT;
		if (val >= VS_FAULT_MAX)
			return -EINVAL;
		mutex_lock(&vs.lock);
		vs.fault = val;
		vs.fault_seq = vs.seq;
		mutex_unlock(&vs.lock);
		break;
	case VSENSOR_IOC_GET_FAULT:
		mutex_lock(&vs.lock);
		val = vs.fault;
		mutex_unlock(&vs.lock);
		if (copy_to_user((u32 __user *)arg, &val, sizeof(val)))
			ret = -EFAULT;
		break;
	case VSENSOR_IOC_RESET:
		mutex_lock(&vs.lock);
		vs_reset_locked();
		mutex_unlock(&vs.lock);
		break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static const struct file_operations vs_fops = {
	.owner          = THIS_MODULE,
	.open           = vs_open,
	.read           = vs_read,
	.unlocked_ioctl = vs_ioctl,
	.llseek         = no_llseek,
};

static struct miscdevice vs_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = VSENSOR_DEV_NAME,
	.fops  = &vs_fops,
	.mode  = 0666,	/* demo convenience: world read/write */
};

static int __init vs_init(void)
{
	int ret;

	mutex_init(&vs.lock);
	vs_reset_locked();
	ret = misc_register(&vs_misc);
	if (ret)
		pr_err("vsensor: misc_register failed (%d)\n", ret);
	else
		pr_info("vsensor: registered /dev/%s\n", VSENSOR_DEV_NAME);
	return ret;
}

static void __exit vs_exit(void)
{
	misc_deregister(&vs_misc);
	pr_info("vsensor: unregistered\n");
}

module_init(vs_init);
module_exit(vs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SensorGuard student project");
MODULE_DESCRIPTION("Virtual temperature sensor with fault injection");
