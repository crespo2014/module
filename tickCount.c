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

static int sys_open(struct inode * i, struct file * f)
{
    struct file_data* pd = kmalloc(sizeof(*pd), GFP_KERNEL);
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
        pd->len = snprintf(pd->buf,sizeof(pd->buf), "%llu \n", x86_getTSC());
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
    kfree(pd);
    f->private_data = NULL;
    return 0;
}

// file operations for misc device
static struct file_operations fops_sys = { //
        .open = sys_open, //
        .read = sys_read, //
        .release = sys_close, //
        };
// misc device resgistration
static struct miscdevice misc = { //
        .name = "tickCount", //
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
    done = 0;
}

/**
 * Module initialization routine
 * @return non zero means module fail
 */
static int init(void)
{
    int r;      // return code of linux functions
    for (;;)
    {
        // register device as miscelanious in sys/devices/misc
        r = misc_register(&misc);
        if (r != 0)
        {
            printk("failed to register misc device %s - code %d\n", misc.name, r);
            break;
        }
        done++;
        return 0;
    }
    cleanup();
    return done;
}

module_init( init);
module_exit( cleanup);

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
