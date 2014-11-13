/*
 * readmap.c
 *
 *  Created on: 13 Nov 2014
 *      Author: lester
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

/**
 * A continuous region is allocated, a counter of many regions is maintenance
 */
struct buff_header
{
  u8 next_index;          // 1 based. 0 is null
  volatile u8 flags;      // 0 empty, 1 Full  ( client only read full block and put empty , produces only read empty and set full )
  volatile u32 dropped;    // how many bytes dropped on this block
  volatile u32 count;     // how many bytes in this block  , if client is fast enough we can switch to other block before fill up this
  u64 data_64[1];         // data is 64 bits alignment
};

// on file open a private data structure create, ioctl allow to set buffer sizes
struct priv_data
{
  void* page;    /// allocate page
  u8 count;    ///< How many pages has been allocate
  u8 mapped;    ///< mmap is in use
};

/**
 * The device is open, it only can be open ones
 *
 */
static int mmap_open(struct inode *inode, struct file *file)
{
  return 0;
}

static int mmap_close(struct inode *inode, struct file *file)
{
  return 0;
}

/**
 * Read operation is execute.
 * If memory is mapped an error is produce, otherwise data is copied
 * there is not reason to wait for the full buffer instead waiting for length is the right way.
 */
static ssize_t mmap_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
  return 0;
}

struct file_operations mmap_fops =    //
    { .read = mmap_read,    //
        .open = mmap_open,    //
        .release = mmap_close, };
