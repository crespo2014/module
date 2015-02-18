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
 *
 * FIXME
 * Driver is not capable of send data without interaction from user space
 * producer will map all memory or just one page.
 * consumer will send data when 4kb is full or half second pass and there is data
 * rd wr pointer will be store in page header
 * driver return number of bytes written
 *
 * User define minimum size of block and how many blocks
 * each block has a header that includes
 * status
 * write position
 * User application is going to jump from one block to another ina cicular way. is the block is not free by kernel then it must wait
 * or allocated more with remap.
 *
 * driver has a table
 * page list.
 * block data. read pointer first page
 *
 * Producer maybe can go to first page if it is free to use.
 * I mean the pages is gone.
 * driver has to check all pages not only the next one
 *
 * how to wit for new data.
 * wr_pos or status is update. check every half second.
 * received a notification everytime a block is wrote through send function
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

#include "queue_mod.h"

#ifdef DEBUG
#define printk_debug(...) printk(__VA_ARGS__)
#else
#define printk_debug(...) do {} while(0)
#endif

enum blck_stat_e
{
    blck_free = 0,
    blck_writting,
    blck_wrote,
};

/// each block has a header
struct block_hdr_t
{
    u8 status;      ///< block status
    u32 wr_pos_;
    void* align;    /// offset of this is start position
};

/*
 * Section information use by the module
 * point to page currently in use
 * page_idx is rd_pos / pagesize. the only thing is that rd_pos does not start at zero at the first
 * page_pos is rd % pagesize;
 */
struct file_data_t
{
    u8 page_count;   /// how many page are allocate
    u8 page_step;  /// how many pages per block, increment block pos using this step
    u8 current_page;   /// current reading block
    u32 rd_pos;         /// current reading pos from current page
    unsigned long *page_ptr;  // it will be more than one, create more pages is possible if this pointer is update
};


int allocate_pages(struct file_data_t* map)
{
    unsigned count = map->page_count * map->page_step;
    map->page_ptr = kmalloc(count*sizeof(void*), GFP_KERNEL);
    memset(map->page_ptr,0,count*sizeof(void*));
    while (count--)
    {
        if ((map->page_ptr[count] = get_zeroed_page(GFP_KERNEL)) == NULL)
            return 1;
    }
    return 0;
}

void deallocate_pages(struct file_data_t* map)
{
    unsigned count = map->page_count * map->page_step;
    while (count--)
    {
        if (map->page_ptr[count] != NULL)
            free_page(map->page_ptr[count]);
    }
    kfree(map->page_ptr);
    //map->page_ptr = NULL;
}

// map function
static int mmap(struct file *fd, struct vm_area_struct *vma)
{
    printk_debug("Queue mmap\n");
    //vma->
    return 0;
}

static int sys_open(struct inode * i, struct file * f)
{
    struct file_data_t* pd;
    printk_debug("Queue sys fs open\n");
    pd = kmalloc(sizeof(struct file_data_t), GFP_KERNEL);
    memset(pd,0,sizeof(*pd));
    return 0;
}

static int sys_read(struct file *f, char __user *data, size_t len, loff_t *offset)
{
    printk_debug("Queue sys fs read\n");
    //f->private_data = kmalloc(sizeof(struct queue),)
    return 0;
}

static int sys_close(struct inode * i, struct file * f)
{
    struct file_data_t* pd;
    printk_debug("Queue sys fs close\n");
    pd = (struct file_data_t*)f->private_data;
    deallocate_pages(pd);
    kfree(pd);
    f->private_data = NULL;
    return 0;
}

long device_ioctl(struct file *file, /* ditto */
         unsigned int ioctl_num,    /* number and param for ioctl */
         unsigned long ioctl_param)
{

    struct queue_info_ q;
    /*
     * Switch according to the ioctl called
     */
    switch (ioctl_num) {
    case QUEUE_INIT:
        get_user(ch,(unsigned char*)ioctl_param);
        tsc_reset(1,ch);
        break;
    case IOCTL_READ_32:
        put_user(read_32(),(uint32_t*)ioctl_param);
        break;
    case IOCTL_READ_64:
        put_user(read_64(),(uint64_t*)ioctl_param);
        break;
    }
    return 0;
}

// file operations for misc device
static struct file_operations fops_sys = { //
        .open = sys_open, //
        .read = sys_read, //
        .mmap = mmap, //
        .unlocked_ioctl = device_ioctl, //
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
    //(done > 1) &&
    if (done > 0)
    {
        r = misc_deregister(&misc);
        if (r != 0)
        {
            printk("failed to deregister misc device %s - code %d\n", misc.name, r);
        }
        else
            printk_debug("deregistered misc device\n");
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
            break;
        }
        printk_debug("registered misc device\n");
        done++;
        return 0;
    }
    cleanup();
    return done;
}

module_init(init);
module_exit(cleanup);

