/*
 * queue_mod.h
 *
 *  Created on: 18 Feb 2015
 *      Author: lester
 */

#ifndef QUEUE_MOD_H_
#define QUEUE_MOD_H_

#define QUEUE_INIT  1

// user configuration send from ioctl
struct queue_info_
{
    u32 block_size;  /// user set max size and read back current size
    u32 block_count; /// user set how many block to allocate
};



#endif /* QUEUE_MOD_H_ */
