/**
 * module.c
 *
 *  Created on: 23 Oct 2014
 *      Author: lester
 *
 */

/**
 * My hello world module
 */

/// kernel headers
#include <linux/kernel.h>
#include <linux/module.h>

/* Deal with CONFIG_MODVERSIONS */
#if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif

/**
 * Module initialization routine
 * @return non zero means module fail
 */
int init_module()
{
  printk("Hello, world - this is the kernel speaking\n");
  return 0;
}

/**
 *  Cleanup - undid whatever init_module did
 */

void cleanup_module()
{
  printk("Short is the life of a kernel module\n");
}
