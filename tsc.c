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

#include "tsc.h"

#ifdef DEBUG
#define printk_debug(...) printk(__VA_ARGS__)
#else
#define printk_debug(...) do {} while(0)
#endif

struct file_data
{
    char buf[30];
    unsigned rd_pos;    // 0 for new read
    unsigned len;       // size of current data
};

static inline uint64_t x86_getTSC(void)        //Time stamp counter
{
    uint64_t ret;
    asm volatile ( "rdtsc" : "=A"(ret) );
    return ret;
}

static inline uint64_t readJiffies_ms(void)
{
    return jiffies * 1000 / HZ;
}

static inline unsigned int arm_getTSC(void)
{
    unsigned int value;
    // Read CCNT Register
    asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));
    return value;
}

static inline void arm_init_TSC(int do_reset, int enable_divider)
{
    // in general enable all counters (including cycle counter)
    int value = 1;

    // peform reset:
    if (do_reset)
    {
        value |= 2;     // reset all counters to zero.
        value |= 4;     // reset cycle counter to zero.
    }

    if (enable_divider)
        value |= 8;     // enable "by 64" divider for CCNT.

    value |= 16;

    // program the performance-counter control-register:
    asm volatile ("MCR p15, 0, %0, c9, c12, 0\t\n" :: "r"(value));

    // enable all counters:
    asm volatile ("MCR p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f));

    // clear overflows:
    asm volatile ("MCR p15, 0, %0, c9, c12, 3\t\n" :: "r"(0x8000000f));
}

static inline void tsc_reset(int reset,int enable_divider)
{
    //arm_init_TSC(reset,enable_divider);
}

static inline uint32_t read_32(void)
{
    //return arm_getTSC();
    return x86_getTSC();
    // return readJiffies_ms();
}

static inline uint64_t read_64(void)
{
    //return arm_getTSC();
     return x86_getTSC();
    // return readJiffies_ms();
}

static int sys_open(struct inode * i, struct file * f)
{
    struct file_data* pd = kmalloc(sizeof(*pd), GFP_KERNEL);
    printk_debug("tsc::open\t");
    f->private_data = pd;
    pd->len = 0;
    pd->rd_pos = 0;
    return 0;
}

static int sys_read(struct file *f, char __user *data, size_t len, loff_t *offset)
{
    unsigned size;
    struct file_data* pd = (struct file_data*)f->private_data;
    if ((pd->rd_pos != 0) && (pd->len == 0))
    {
        // A number was send
        pd->rd_pos = 0;
        return 0;
    }
    if (pd->len == 0)
    {
        pd->len = snprintf(pd->buf,sizeof(pd->buf), "%llu \n", read_64());
        pd->rd_pos = 0;
    }
    size = (len > pd->len) ? pd->len : len;
    if (copy_to_user(data,pd->buf + pd->rd_pos,size) != 0)
    {
        printk("failed to copy to user \n");
    }
    pd->rd_pos += size;
    pd->len -= size;
    return size;
}

static int sys_close(struct inode * i, struct file * f)
{
    struct queue* pd = (struct queue*) f->private_data;
    printk_debug("tsc::close\t");
    kfree(pd);
    f->private_data = NULL;
    return 0;
}

long device_ioctl(struct file *file, /* ditto */
         unsigned int ioctl_num,    /* number and param for ioctl */
         unsigned long ioctl_param)
{
    uint8_t ch;
    u32 v32;
    u64 v64;
    printk_debug("tsc::ioctl %d\n",ioctl_num);
    /*
     * Switch according to the ioctl called
     */
    switch (ioctl_num) {
    case IOCTL_RESET:
        get_user(ch,(unsigned char*)ioctl_param);
        tsc_reset(1,ch);
        break;
    case IOCTL_READ_32:
        v32 = read_32();
        if (copy_to_user((void*)ioctl_param,&v32,sizeof(v32)) !=0)
        {
            printk_debug("tsc copy to user failed\n");
            return EFAULT;
        }
        break;
    case IOCTL_READ_64:
        v64 = read_64();
        if (copy_to_user((void*)ioctl_param,&v64,sizeof(v64)) !=0)
        {
            printk_debug("tsc copy to user failed\n");
            return EFAULT;
        }
        break;
    default:
        return EINVAL;
    }
    return 0;
}

// file operations for misc device
static struct file_operations fops_sys = { //
        .open = sys_open, //
        .read = sys_read, //
        .unlocked_ioctl = device_ioctl,
        .release = sys_close, //
        };
// misc device resgistration
static struct miscdevice misc = { //
        .name = "tsc", //
        .fops = &fops_sys, //
        };


enum init_level { none,misc_reg,all};

static void clean(enum init_level lvl)
{
    int r;
    switch (lvl)
    {
    case all:
    case misc_reg:
        if ((r = misc_deregister(&misc)) != 0)
        {
            printk("failed to deregister misc device %s - code %d\n", misc.name, r);
        }
    default:
        return;
    }
}

static void __exit cleanup(void)
{
    clean(all);
}

/**
 * Module initialization routine
 * @return non zero means module fail
 */
static int __init init(void)
{
    enum init_level done = none;  // how far the module has been initialize
    int r;      // return code of linux functions
    // register device as miscelanious in sys/devices/misc
    r = misc_register(&misc);
    if (r != 0)
    {
        printk("failed to register misc device %s - code %d\n", misc.name, r);
        goto clean_all;
    }
    done = misc_reg;
    //arm_init_TSC(1,1);
    return 0;

clean_all:
    clean(done);
    return r;
}

module_init( init);
module_exit( cleanup);

MODULE_DESCRIPTION("Time stamp counter reader module.");
MODULE_LICENSE("GPL");

/*
 void inline Filltimes(uint64_t **times) {
 unsigned long flags;
 int i, j;
 uint64_t start, end;
 unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
 volatile int variable = 0;

 asm volatile ("CPUID\n\t"
 "RDTSC\n\t"
 "mov %%edx, %0\n\t"
 "mov %%eax, %1\n\t"
 : "=r" (cycles_high), "=r" (cycles_low)
 :
 :"%rax", "%rbx", "%rcx","%rdx");
 asm volatile ("CPUID\n\t"
 "RDTSC\n\t"
 "CPUID\n\t"
 "RDTSC\n\t"
 "mov %%edx, %0\n\t"
 "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx",
 "%rdx");
 asm volatile ("CPUID\n\t"
 "RDTSC\n\t"::: "%rax", "%rbx", "%rcx", "%rdx");


 for (j=0; j<BOUND_OF_LOOP; j++) {
 for (i =0; i<SIZE_OF_STAT; i++) {

 variable = 0;

 preempt_disable();
 raw_local_irq_save(flags);

 asm volatile (
 "CPUID\n\t"
 "RDTSC\n\t"
 "mov %%edx, %0\n\t"
 "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx",
 "%rdx");

 asm volatile(
 "CPUID\n\t"
 "RDTSC\n\t"
 "mov %%edx, %0\n\t"
 "mov %%eax, %1\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx",
 "%rdx");

 raw_local_irq_restore(flags);
 preempt_enable();
 */
