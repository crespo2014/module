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


// page list structure



