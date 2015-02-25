/*
 * tsc.h
 *
 *  Created on: 22 Jan 2015
 *      Author: lester
 */

#ifndef TSC_H_
#define TSC_H_

#include <linux/ioctl.h>
#include <linux/kdev_t.h> /* for MKDEV */

#define DEVICE_NAME "timeStampCounter"
#define DEVICE_PATH "/dev/tsc"
#define MAGIC_NO 4

/*
 * Set the message of the device driver
 */
#define IOCTL_RESET     _IO(MAGIC_NO, 0)
#define IOCTL_READ_32   _IOR(MAGIC_NO, 1, char *)
#define IOCTL_READ_64   _IOR(MAGIC_NO, 2, char *)


#endif /* TSC_H_ */
