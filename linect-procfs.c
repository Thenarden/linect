/** 
 * @file linect-procfs.c
 * @author Arturo Casal
 * @date 2010
 * @version v0.1
 *
 * @brief Driver for MS Kinect
 *
 * @note Copyright (C) Arturo Casal
 *
 * @par Licences
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/kref.h>
#include <linux/mm.h>

#include <linux/usb.h>
#include <media/v4l2-common.h>
#include <media/v4l2-ioctl.h>

#include <linux/string.h>

#include <linux/proc_fs.h>

#include "linect.h"

extern int kindex;
extern int nmotors;

extern struct proc_dir_entry *linect_proc_root_entry;

extern struct usb_device *tmp_motor_dev;
extern struct mutex modlock_tmpmotor;
extern struct mutex modlock_proc;


//static int proc_read_led(char *page, char **start, off_t off, int count, int *eof, void *data);
static ssize_t proc_read_led(struct file *filp, char __user *data, size_t count, loff_t *offp);
//static int proc_write_led(struct file *file, const char *buffer, unsigned long count, void *data);
static ssize_t proc_write_led (struct file* file, const char __user* buffer, size_t count, loff_t* data);

//static int proc_read_motor(char *page, char **start, off_t off, int count, int *eof, void *data);
static ssize_t proc_read_motor(struct file *filp, char __user *data, size_t count, loff_t *offp);
//static int proc_write_motor_raw(struct file *file, const char *buffer, unsigned long count, void *data);
static ssize_t proc_write_motor_raw (struct file* file, const char __user* buffer, size_t count, loff_t* data);
//static int proc_write_motor_char(struct file *file, const char *buffer, unsigned long count, void *data);
static ssize_t proc_write_motor_char (struct file* file, const char __user* buffer, size_t count, loff_t* data);

//static int proc_read_accel_char(char *page, char **start, off_t off, int count, int *eof, void *data);
static ssize_t proc_read_accel_char(struct file *filp, char __user *data, size_t count, loff_t *offp);
//static int proc_read_accel_raw(char *page, char **start, off_t off, int count, int *eof, void *data);
static ssize_t proc_read_accel_raw(struct file *filp, char __user *data, size_t count, loff_t *offp);

static struct file_operations proc_ops_mot_char = { 
	.read = proc_read_motor, 
	.write = proc_write_motor_char 
};

static struct file_operations proc_ops_mot_raw = { 
	.read = proc_read_motor, 
	.write = proc_write_motor_raw 
};

static struct file_operations proc_ops_led = {
	.read = proc_read_led,
	.write = proc_write_led,
};

static struct file_operations proc_ops_accel_char = {
	.read = proc_read_accel_char,
};

static struct file_operations proc_ops_accel_raw = {
	.read = proc_read_accel_raw,
};

void linect_proc_create(struct usb_linect *dev)
{
	char dirname[10];
	struct proc_dir_entry *pde;
	
	LNT_DEBUG("Creating motor %d /proc. Nmotors = %d\n", dev->index, nmotors);
	
	// Root
	mutex_lock(&modlock_proc);
		if (linect_proc_root_entry == NULL && nmotors==0)
			linect_proc_root_entry = proc_mkdir("linect", NULL);
	
	// Dev root
	snprintf(dirname, 10, "%d", dev->index);
	dev->proc_root = proc_mkdir(dirname, linect_proc_root_entry);
	
	// Dev motor char
	pde = proc_create_data("motor_char", 0666, dev->proc_root, &proc_ops_mot_char, dev);
//	pde->data = dev;
//	pde->read_proc = proc_read_motor;
//	pde->write_proc = proc_write_motor_char;
	
	// Dev motor char
	pde = proc_create_data("motor_raw", 0666, dev->proc_root, &proc_ops_mot_raw, dev);
//	pde->data = dev;
//	pde->read_proc = proc_read_motor;
//	pde->write_proc = proc_write_motor_raw;
	
	// Dev led
	pde = proc_create_data("led", 0666, dev->proc_root, &proc_ops_led, dev);
//	pde->data = dev;
//	pde->read_proc = proc_read_led;
//	pde->write_proc = proc_write_led;
	
	// Dev accel
	pde = proc_create_data("accel_char", 0444, dev->proc_root, &proc_ops_accel_char, dev);
//	pde->data = dev;
//	pde->read_proc = proc_read_accel_char;
	
	pde = proc_create_data("accel_raw", 0444, dev->proc_root, &proc_ops_accel_raw, dev);
//	pde->data = dev;
//	pde->read_proc = proc_read_accel_raw;
	
	// Dev motor move
	
	mutex_unlock(&modlock_proc);

}

void linect_proc_destroy(struct usb_linect *dev)
{
	char dirname[10];
	
	snprintf(dirname, 10, "%d", dev->index);
	
	mutex_lock(&modlock_proc);
	
	// Dev root
	if (dev && dev->proc_root) {
		remove_proc_entry("motor_raw", dev->proc_root);
		remove_proc_entry("motor_char", dev->proc_root);
		remove_proc_entry("led", dev->proc_root);
		remove_proc_entry("accel_char", dev->proc_root);
		remove_proc_entry("accel_raw", dev->proc_root);
		if (linect_proc_root_entry)
			remove_proc_entry(dirname, linect_proc_root_entry);
		dev->proc_root = NULL;
	}
	
	// Root
	if (nmotors==0) {
		remove_proc_entry("linect", NULL);
		linect_proc_root_entry = NULL;
	}
	
	mutex_unlock(&modlock_proc);

}

//static int proc_read_led(char *page, char **start, off_t off, int count, int *eof, void *data)
static ssize_t proc_read_led(struct file *filp, char __user * buffer, size_t count, loff_t *data)
{
	int len;
	struct usb_linect* dev;
	
	dev = (struct usb_linect*)data;

	len = sprintf(buffer, "values:\n0 = off\n1 = green\n2 = red\n3 = yellow\n4 = blink yellow\n5 = blink green\n6 = blink red yellow\n");

	return len;
}

//static int proc_write_led(struct file *file, const char *buffer, unsigned long count, void *data)
static ssize_t proc_write_led (struct file* file, const char __user* buffer, size_t count, loff_t* data)
{
	int len;
	struct usb_linect *dev;
	unsigned int led;
	int ret;

	dev = (struct usb_linect*)data;
	led = 0;
	
	if (count < 1) return -EINVAL;
	
	len = copy_from_user(&led, buffer, 1);

	if ((led >= '0') && (led <= '9'))
		led -= '0';
		
	if (led >= 0 && led <= 6) {
		LNT_INFO("Set led to %d \n", led);
		ret = linect_motor_set_led(dev, led);

		if (ret < 0)
		{
			ret *= -1;


			LNT_INFO("ERROR: %d \n", ret);
		}
		else
		{
			LNT_INFO("LED status set. \n");
		}
	} else {
		return -EINVAL;
	}

	return count;
}

//static int proc_read_motor(char *page, char **start, off_t off, int count, int *eof, void *data)
static ssize_t proc_read_motor(struct file *filp, char __user *buffer, size_t count, loff_t *data)
{
	int len;

	len = sprintf(buffer, "range = [0, 62];\nup = 62;\ndown = 0;\nmiddle = 31;\n");
	
	return len;
}

//static int proc_write_motor_raw(struct file *file, const char *buffer, unsigned long count, void *data)
static ssize_t proc_write_motor_raw (struct file* file, const char __user* buffer, size_t count, loff_t* data)
{
	int len;
	struct usb_linect *dev;
	unsigned int deg;

	dev = (struct usb_linect*)data;
	deg = 0;
	
	if (count < 1) return -EINVAL;
	
	len = copy_from_user(&deg, buffer, 1);
	
	//led >>= 23;
	
	LNT_INFO("Set motor angle to: %d \n", deg);
		
	if (deg < 0 || deg > 62) {
		return -EINVAL;
	}
	
	linect_motor_set_tilt_degs(dev, (int)deg-31);

	return count;
}

//static int proc_write_motor_char(struct file *file, const char *buffer, unsigned long count, void *data)
static ssize_t proc_write_motor_char (struct file* file, const char __user* buffer, size_t count, loff_t* data)
{
	int len;
	struct usb_linect *dev;
	char deg_chr[3];
	unsigned int deg;

	dev = (struct usb_linect*)data;
	
	if (count < 1) return -EINVAL;
	
	memset(deg_chr, 0x00, 3);
	len = (count == 1) ? 1 : 2;
	len = copy_from_user(deg_chr, buffer, len);
	
	deg = (unsigned int)simple_strtol(deg_chr, NULL, 10);
		
	LNT_INFO("Set motor angle to: %d \n", deg);
		
	if (deg < 0 || deg > 62) {
		return -EINVAL;
	}
	
	linect_motor_set_tilt_degs(dev, (int)deg-31);

	return count;
}

//static int proc_read_accel_char(char *page, char **start, off_t off, int count, int *eof, void *data)
static ssize_t proc_read_accel_char(struct file *filp, char __user *buffer, size_t count, loff_t *data)
{
	int len;
	struct usb_linect *dev;
	uint16_t x, y, z;

	dev = (struct usb_linect*)data;
	
	linect_get_raw_accel(dev, &x, &y, &z);

	len = sprintf(buffer, "%d %d %d\n", x, y, z);
	
	return len;
}

//static int proc_read_accel_raw(char *page, char **start, off_t off, int count, int *eof, void *data)
static ssize_t proc_read_accel_raw(struct file *filp, char __user *buffer, size_t count, loff_t *data)
{
	int len;
	struct usb_linect *dev;
	uint16_t pts[3];

	dev = (struct usb_linect*)data;
	
	linect_get_raw_accel(dev, &pts[0], &pts[1], &pts[2]);
	
	len = count < sizeof(uint16_t)*3 ? count : sizeof(uint16_t)*3;
	
	len = copy_to_user(filp, pts, len);
	
	return len;
}

