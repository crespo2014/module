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

#ifndef u8
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#endif


#define DEVICE_NAME "KernelQueue"
#define DEVICE_PATH "/dev/queue"
#define MAGIC_NO 10


/**
 * Block status
 */
enum blck_stat_e
{
    blck_free = 0,
    blck_writting,
    blck_wrote,
};

// user configuration send from ioctl
struct queue_info_
{
    volatile u32 block_size;  /// user set max size and read back current size
    volatile u32 block_count; /// user set how many block to allocate
};

/// each block has a header
struct block_hdr_t
{
    volatile u8 status;      ///< block status
    volatile u16 wr_pos_;
    void* align;    /// offset of this is start position
};

#define QUEUE_INIT    _IOWR(MAGIC_NO, 0,struct queue_info_)

#endif /* QUEUE_MOD_H_ */
