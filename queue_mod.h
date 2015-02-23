/*
 * queue_mod.h
 *
 *  Created on: 18 Feb 2015
 *      Author: lester
 */

#ifndef QUEUE_MOD_H_
#define QUEUE_MOD_H_

#define QUEUE_INIT  1

#ifndef u8
#define u8 uint8_t
#define u32 uint32_t
#endif

// user configuration send from ioctl
struct queue_info_
{
    u32 block_size;  /// user set max size and read back current size
    u32 block_count; /// user set how many block to allocate
};

/// each block has a header
struct block_hdr_t
{
    u8 status;      ///< block status
    u32 wr_pos_;
    void* align;    /// offset of this is start position
};


#endif /* QUEUE_MOD_H_ */
