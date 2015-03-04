/*
 * queue_mod.h
 *
 *  Created on: 18 Feb 2015
 *      Author: lester
 */

#ifndef QUEUE_MOD_H_
#define QUEUE_MOD_H_

#include <linux/ioctl.h>
#include <linux/kdev_t.h> /* for MKDEV */
#include <linux/types.h>

#define DEVICE_NAME "KernelQueue"
#define DEVICE_PATH "/dev/queue"
#define MAGIC_NO 10


/**
 * Block status
 */
enum blck_stat_e
{
    blck_free = 0,
    blck_writting,      // user writing this block
    blck_wrote,         // user finish with this block
    blck_kernel_lock,   // kernel leave this block but could be lock if ref count is not 0

};

// user configuration send from ioctl
struct queue_info_
{
    volatile __u32 block_size;  /// user set max size and read back current size
    volatile __u32 block_count; /// user set how many block to allocate
    volatile __u32 block_start_offset;  //space need by kernel
};

/*
 * timeouts configuration structure
 */
struct queue_timeouts_t
{
    __u32 read_timeout_ms;
    __u32 write_tieout_ms;
};

/// each block has a header
struct block_hdr_t
{
    volatile __u8 status;      ///< block status
    volatile __u16 wr_pos_;
};

#define QUEUE_INIT    _IOWR(MAGIC_NO, 0,struct queue_info_)

#endif /* QUEUE_MOD_H_ */
