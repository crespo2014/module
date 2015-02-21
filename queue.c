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

/*
 * Section information use by the module
 * point to page currently in use
 * page_idx is rd_pos / pagesize. the only thing is that rd_pos does not start at zero at the first
 * page_pos is rd % pagesize;
 */
struct queue_t
{
    u8 page_count;   /// how many page are allocate
    u8 page_step;  /// how many pages per block, increment block pos using this step
    u8 current_page;   /// current reading block
    u32 rd_pos;         /// current reading pos from current page
    unsigned long *page_ptr;  // it will be more than one, create more pages is possible if this pointer is update
    wait_queue_head_t rd_queue,wr_queue;       /* read and write wait queues for blocking operations */
};


int allocate_pages(struct queue_t* map)
{
    unsigned count = map->page_count * map->page_step;
    map->page_ptr = kmalloc(count*sizeof(void*), GFP_KERNEL);
    memset(map->page_ptr,0,count*sizeof(void*));
    while (count--)
    {
        if ((map->page_ptr[count] = get_zeroed_page(GFP_KERNEL)) == 0)
            return 1;
    }
    return 0;
}

void deallocate_pages(struct queue_t* map)
{
    unsigned count = map->page_count * map->page_step;
    while (count--)
    {
        if (map->page_ptr[count] != 0)
            free_page(map->page_ptr[count]);
    }
    kfree(map->page_ptr);
    //map->page_ptr = NULL;
}

// map function
static int device_mmap(struct file *fd, struct vm_area_struct *vma)
{
    struct queue_t* pd = (struct queue_t*)fd->private_data;
    // trace all information
    printk_debug("mmap request from 0x%X to 0x%X\n",vma->vm_start,vma->vm_end);

    if (pd->page_count == 0)
    {
        printk_debug("Missing Queue init call");
        return 2;
    }
    if (pd->page_ptr != NULL)
    {
        printk_debug("Queue already map");
        return 2;
    }
    allocate_pages(pd);
    printk_debug("Queue device_mmap\n");
    return 0;
}

static int device_open(struct inode * i, struct file * f)
{
    struct queue_t* pd;
    pd = kmalloc(sizeof(struct queue_t), GFP_KERNEL);
    memset(pd,0,sizeof(*pd));
    return 0;
}

static int device_close(struct inode * i, struct file * f)
{
    struct queue_t* pd;
    printk_debug("Queue sys fs close\n");
    pd = (struct queue_t*)f->private_data;
    deallocate_pages(pd);
    kfree(pd);
    f->private_data = NULL;
    return 0;
}

long device_ioctl(struct file *file, /* ditto */
         unsigned int ioctl_num,    /* number and param for ioctl */
         unsigned long ioctl_param)
{
    struct queue_t* pd = (struct queue_t*)file->private_data;
    struct queue_info_ q;
    /*
     * Switch according to the ioctl called
     */
    switch (ioctl_num) {
    case QUEUE_INIT:
        if (pd->page_ptr != NULL)
        {
            printk_debug("Queue init called twices");
            return 2;
        }
        if (copy_from_user(&q,(void*)ioctl_param,sizeof(struct queue_info_)) !=0 )
        {
            return 4;
        }
        if (q.block_size % PAGE_SIZE)
                q.block_size += (PAGE_SIZE - q.block_size % PAGE_SIZE);// ((q.block_size+page_size() - 1) / page_size;
        if (copy_to_user((void*)ioctl_param,&q,sizeof(struct queue_info_)) != 0)
        {
            return 4;
        }
        pd->page_step = q.block_size / PAGE_SIZE;
        pd->page_count = pd->page_step * q.block_count;
        pd->current_page = 0;
        pd->rd_pos = 6;     //FIXME
        break;
    }

    return 0;
}

/*
 * Write operation still supported.
 * If null pointer is received this operation is treated as a flush called every a block is full
 */
static int device_write(struct file *fd,const char __user *data, size_t len, loff_t *offset)
{
    struct queue_t* pd = (struct queue_t*)fd->private_data;
    if (data != NULL)
    {
        // fill current written block and append data must be space for all or nothing
    }
    else
    {
        // do a flush and notify to read side to keep reading
        //pd->
    }
    return pd->rd_pos;
}


/*
 * To be implemented read operation
 * also support for splice is needed as well
 */
static int device_read(struct file *fd, char __user *data, size_t len, loff_t *offset)
{
    struct queue_t* pd = (struct queue_t*)fd->private_data;
    printk_debug("Queue read\n");

    return pd->rd_pos;
}

// file operations for misc device
static struct file_operations fops_sys = { //
        .open = device_open, //
        .read = device_read, //
        .write = device_write, //
        .mmap = device_mmap, //
        .unlocked_ioctl = device_ioctl, //
        .release = device_close, //
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

