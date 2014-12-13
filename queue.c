/**
 * queue.c
 *
 *  Created on: 9 Dec 2014
 *      Author: lester
 *
 * This module create a queue in kernel space
 *
 * User interface open the driver.
 * use mmap to request memory.
 * first 32 bits of mapped memory count how many bytes has been written
 * when more memory is needed the current is unmap and new memory is requested
 *
 * user need to ask for more memory than requested.
 * when memory is unmapped all consumed block and not consumed are reuse.
 *
 * driver will control current page offset + read position that it is less than writeen
 * starting from offset 4 , pos 0,
 *
 * current sending page. the list is holding in kernel. if read = write then pages ahead are free.
 * to find last used page get remaining and walk through all pages until 0
 */

/// kernel headers
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

// page list structure
struct page_item
{
    void* page;
    struct page_item* next;
};

struct queue
{
    struct page_item *free_pages;   // availabel pages
    struct page_item *map_pages;    // pages currently mapped
    unsigned rd_pos;    // golbal read pos
    struct page_item *rd_page;         // current readign page
    unsigned rd_offset;     // offset in current reading page, max == pagesize
};


// map function
static int mmap(struct file *fd, struct vm_area_struct *vma)
{
    struct page *page;
    struct mmap_info *info;
    return 0;
}

static int sys_open(struct inode * i, struct file * f)
{
    printk("Queue sys fs open");
    //f->private_data = kmalloc(sizeof(struct queue),)
    return 0;
}

static int sys_read(struct file *f, char __user *data, size_t len, loff_t *offset)
{
    printk("Queue sys fs read");
    //f->private_data = kmalloc(sizeof(struct queue),)
    return 0;
}

static int sys_close(struct inode * i, struct file * f)
{
    printk("Queue sys fs close");
    //f->private_data = kmalloc(sizeof(struct queue),)
    return 0;
}

// file operations for misc device
static struct file_operations fops_sys = { //
        .open = sys_open, //
        .read = sys_read, //
        .mmap = mmap, //
        .release = sys_close, //
        };
// misc device resgistration
static struct miscdevice misc = { //
        .name = "queue", //
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
}

/**
 * Module initialization routine
 * @return non zero means module fail
 */
static int init(void)
{
    int r;		// return code of linux functions
    for (;;)
    {
        // register device as miscelanious in sys/devices/misc
        r = misc_register(&misc);
        if (r != 0)
        {
            printk("failed to register misc device %s - code %d\n", misc.name, r);
        }
        done++;
        return 0;
    }
    cleanup();
    return done;
}

module_init(init);
module_exit(cleanup);

