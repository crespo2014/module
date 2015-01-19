/*
 * tickCount.c
 *
 * Module to get precisse time counter in nanoseconds
 * On open or write the counter is reset to zero
 * ioctrl functions to read and reset also available
 *
 * file struct is a char buffer with the number plus current position to be read and size.
 *
 *
 *  Created on: 17 Jan 2015
 *      Author: lester
 *
 *
 */


#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/debugfs.h>
#include <linux/mm.h>  // mmap related stuff
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>

#ifdef DEBUG
#define printk_debug(...) printk(__VA_ARGS__)
#else
#define printk_debug(...) do {} while(0)
#endif

struct file_data
{
    char buf[20];
    unsigned rd_pos;    // 0 for new read
    unsigned len;       // size of current data
    unsigned long start_time;
};

static int sys_open(struct inode * i, struct file * f)
{
    struct file_data* pd = kmalloc(sizeof(*pd), GFP_KERNEL);
    f->private_data = pd;
    pd->len = 0;
    pd->rd_pos = 0;
    pd->start_time = jiffies;
    return 0;
}

static int sys_read(struct file *f, char __user *data, size_t len, loff_t *offset)
{
    struct file_data* pd = (struct file_data*)f->private_data;
    if ((pd->rd_pos != 0) && (pd->len == 0))
    {
        // A number was send
        pd->rd_pos = 0;
        return 0;
    }
    if (pd->len == 0)
    {
        unsigned long nsec = ((long)jiffies - (long)pd->start_time)/HZ;
        pd->len = snprintf(pd->buf,sizeof(pd->buf),"%lu \n",jiffies);
        pd->rd_pos = 0;
    }
    unsigned l;
    l = (len > pd->len) ? pd->len : len;
    if (copy_to_user(data,pd->buf + pd->rd_pos,l) != 0)
    {
        printk("failed to copy to user \n");
    }
    pd->rd_pos += l;
    pd->len -= l;
    return l;
}

static int sys_close(struct inode * i, struct file * f)
{
    struct queue* pd = (struct queue*)f->private_data;
    kfree(pd);
    f->private_data = NULL;
    return 0;
}

// file operations for misc device
static struct file_operations fops_sys = { //
        .open = sys_open, //
        .read = sys_read, //
        .release = sys_close, //
        };
// misc device resgistration
static struct miscdevice misc = { //
        .name = "tickCount", //
        .fops = &fops_sys, //
        };

static unsigned done = 0;  // how far the module has been initialize

static void cleanup(void)
{
    int r;
    if (done > 0)
    {
        r = misc_deregister(&misc);
        if (r != 0)
        {
            printk("failed to deregister misc device %s - code %d\n", misc.name, r);
        }
    }
    done = 0;
}

/**
 * Module initialization routine
 * @return non zero means module fail
 */
static int init(void)
{
    int r;      // return code of linux functions
    for (;;)
    {
        // register device as miscelanious in sys/devices/misc
        r = misc_register(&misc);
        if (r != 0)
        {
            printk("failed to register misc device %s - code %d\n", misc.name, r);
            break;
        }
        done++;
        return 0;
    }
    cleanup();
    return done;
}

module_init(init);
module_exit(cleanup);

