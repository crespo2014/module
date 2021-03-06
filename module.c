/**
 * module.c
 *
 *  Created on: 23 Oct 2014
 *      Author: lester
 *
 * scatter/gather read and write operations
 *
 * write and read using list of buffers with size.
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
//#include <linux/wrapper.h>

/**
 * A continuous region is allocated, a counter of many regions is maintenance
 */
struct buff_header
{
	u8 next_index;          // 1 based. 0 is null
	volatile u8 flags; // 0 empty, 1 Full  ( client only read full block and put empty , produces only read empty and set full )
	volatile u32 dropped;    // how many bytes dropped on this block
	volatile u32 count; // how many bytes in this block  , if client is fast enough we can switch to other block before fill up this
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
	//get_page();

	return 0;
}

static int mmap_close(struct inode *inode, struct file *file)
{
	return 0;
}

static int mmap_mmap(struct inode *inode, struct file *file)
{
	//virt_to_page();
	return 0;
}

static struct file_operations fops =
{ };
static struct miscdevice misc =
{ };

/**
 * Function to handler mmap calls from userspace
 */
static int fop_mmap(struct file *fd, struct vm_area_struct *vma)
{
	struct page *page;
	struct mmap_info *info;
	/* is the address valid? */
//	if (address > vma->vm_end) {
//		printk("invalid address\n");
//		return NOPAGE_SIGBUS;
//	}
//	/* the data is in vma->vm_private_data */
//	info = (struct mmap_info *)vma->vm_private_data;
//	if (!info->data) {
//		printk("no data\n");
//		return NULL;
//	}
//
//	/* get the page */
//	page = virt_to_page(info->data);
//
//	/* increment the reference count of this page */
//	get_page(page);
//	/* type is the page fault type */
//	if (type)
//		*type = VM_FAULT_MINOR;

	return 0;
}

/**
 * Function to handler device open
 */
int fop_open(struct inode *inode, struct file *filp)
{
	struct priv_data *ppd = kmalloc(sizeof(struct priv_data), GFP_KERNEL);
	filp->private_data = ppd;
	return 0;
}
/**
 * register module on sys/devices/misc automatically  a dev node will be create
 * @return 0 operation success
 */
static int sys_fs_register(void)
{
	fops.mmap = fop_mmap;
	misc.name = "mapTest";
	misc.fops = &fops;
	return misc_register(&misc);
}

/**
 * deregister driver from sys fs. check driver ref count
 */
static void sys_fs_deregister(void)
{
	misc_deregister(&misc);
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
		{ .mmap = mmap_mmap, .read = mmap_read,    //
				.open = mmap_open,    //
				.release = mmap_close,
		//.poll .flush .sendpage .splice_read
		};

///* Deal with CONFIG_MODVERSIONS */
//#if CONFIG_MODVERSIONS==1
//#define MODVERSIONS
//#include <linux/modversions.h>
//#endif

/* The name for our device, as it will appear in /proc/devices */
#define DEVICE_NAME "my_char_dev"

#define BUF_LEN 80  ///< max message length
static int open_ = 0;    ///<open only ones
static char message_[BUF_LEN];    ///< message to reply
static char *message_pos = message_;        ///<current message send position

static int Major = 0;

#define procfs_name "helloworld"

// Putting device on /proc file system

/**
 * function to be called when a read on proc is done
 *  buffer can be use or a new one can be returned
 *  @return number of bytes filled 0 - means end of file
 */
int procfile_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int zero)
{
	int len;
	static char my_buffer[80];
	static int count = 1;
// return all information in one go
	if (offset > 0)
		return 0;
	/* Fill the buffer and get its length */
	len = sprintf(my_buffer, "For the %d%s time, go away!\n", count,
			(count % 100 > 10 && count % 100 < 14) ? "th" : (count % 10 == 1) ? "st" : (count % 10 == 2) ? "nd" : (count % 10 == 3) ? "rd" : "th");
	count++;
	/* Tell the function which called us where the
	 * buffer is */
	*buffer_location = my_buffer;
	/* Return the length */
	return len;
}

/**
 * Function call when a process open the file
 */
static int device_open(struct inode *inode, struct file *file)
{
	static int counter = 0;
#ifdef DEBUG
	printk ("device_open(%p,%p)\n", inode, file);
#endif
	printk("Device: %d.%d\n", inode->i_rdev >> 8, inode->i_rdev & 0xFF);
	if (open_)
		return -EBUSY;

	open_++;
	/* Initialize the message. */
	sprintf(message_, "If I told you once, I told you %d times - %s", counter++, "Hello, world\n");

	message_pos = message_;

	return 0;
}
/**
 * function called when a process close the file
 */
static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk ("device_release(%p,%p)\n", inode, file);
#endif
	open_--;
	return 0;
}

/* This function is called whenever a process which
 * have already opened the device file attempts to
 * read from it. */
static ssize_t device_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
	int bytes_read = 0;
	static int counter = 0;
	sprintf(message_, "If I told you once, I told you %d times - %s", counter++, "Hello, world\n");
	/* Number of bytes actually written to the buffer */

	/* If we’re at the end of the message, return 0
	 * (which signifies end of file) */
	if (*message_pos == 0)
	{
		message_pos = message_;
		return 0;
	}
	/* Actually put the data into the buffer */
	while (length && *message_pos)
	{
		/* Because the buffer is in the user data segment,
		 * not the kernel data segment, assignment wouldn’t
		 * work. Instead, we have to use put_user which
		 * copies data from the kernel data segment to the
		 * user data segment. */
		put_user(*(message_pos++), buffer++);
		length--;
		bytes_read++;
	}
#ifdef DEBUG
	printk ("Read %d bytes, %d left\n",
			bytes_read, length);
#endif
	/* Read functions are supposed to return the number
	 * of bytes actually inserted into the buffer */
	return bytes_read;
}

/* This function is called when somebody tries to write
 * into our device file - unsupported in this example. */
static ssize_t device_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
	return -EINVAL;
}

/*
 *
 */
struct file_operations Fops =
{ .read = device_read,    //
		.write = device_write,    //
		.open = device_open,    //
		.release = device_release, };

/**
 * chardev modules need to register the module
 * cat /proc/devies wil produce
 * Character devices:
 * 251 my_char_dev
 *
 * create a device mknod test char 251 1
 * cat /dev/test
 *
 * if module is remove then /dev/test will stop working
 *
 *
 */
int char_dev_init(void)
{
	/* Register the character device (atleast try) */
	Major = register_chrdev(0, DEVICE_NAME, &Fops);
	/* Negative values signify an error */
	if (Major < 0)
	{
		printk("%s device failed with %d\n", "Sorry, registering the character", Major);
		return Major;
	}
	printk("%s The major device number is %d.\n", "Registeration is a success.", Major);
	printk("If you want to talk to the device driver,\n");
	printk("you will have to create a device file. \n");
	printk("mknod <name> c %d <minor>\n", Major);
	return 0;
}
/**
 * Unregister the device
 */
void char_dev_clean(void)
{
	sys_fs_deregister();
	if (Major != 0)
	{
		unregister_chrdev(Major, DEVICE_NAME);
	}
}

/**
 * Proc file system struct
 */
static const struct file_operations proc_fops_ =
{ .owner = THIS_MODULE, .read = device_read, .llseek = noop_llseek, };

struct proc_dir_entry *entry = NULL;

int proc_fs_init(void)
{
	entry = proc_create(procfs_name, 0644, NULL, &proc_fops_);

	if (entry == NULL)
		return -ENOMEM;

//  if (Our_Proc_File == NULL)
//  {
//    remove_proc_entry(procfs_name, &proc_root);
//    printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
//    procfs_name);
//    return -ENOMEM;
//  }
//
//  Our_Proc_File->read_proc = procfile_read;
//  Our_Proc_File->owner = THIS_MODULE;
//  Our_Proc_File->mode = S_IFREG | S_IRUGO;
//  Our_Proc_File->uid = 0;
//  Our_Proc_File->gid = 0;
//  Our_Proc_File->size = 37;

	printk(KERN_INFO "/proc/%s created\n", procfs_name);
	return 0; /* everything is ok */
}

/**
 * Release entry on /proc filesystem
 */
void proc_fs_release(void)
{
	if (entry != NULL)
	{
		remove_proc_entry(procfs_name, entry);
	}
}

/**
 * Module initialization routine
 * @return non zero means module fail
 */
static int init(void)
{
	//printk("Hello, world - this is the kernel speaking\n");
	if (sys_fs_register() != 0)
		goto sys_fs_failed;
	printk("sys_fs_register\n");
//	for (;;)
//	{
//		if (char_dev_init() != 0)
//			break;
//		if (proc_fs_init() != 0)
//			break;
//		return 0;
//	}
//	proc_fs_release();
//	char_dev_clean();
	printk("init done\n");
	return 0;
	sys_fs_failed:

	return -1;
}

/**
 *  Cleanup - undid whatever init_module did
 */
static void cleanup(void)
{
	sys_fs_deregister();
	printk("sys_fs_deregister\n");
	//char_dev_clean();
}

module_init(init);
module_exit(cleanup);

/**
 * Queue module tips
 * Operation modes
 * 1 - driver only send out full 4KB pages.
 * 2 - Driver send any data each second
 * send when page is full or one second after last send
 *
 * Driver is holding many lists of pages
 * free_page list.		// we do not need page list because pages are only one by one
 *   page*
 *   next*
 *
 * user map
 *   page*	//first page contains wr pointer as header. use offset to send out this page
 *   next*
 *   read_pos  //
 *
 * pending
 *   page*
 *   next*
 *   begin		// how much send for this page
 *   end		// use it when page == null otherwise send full page from begin
 *
 * user request a map of memory.
 *   more size that request is return.
 *   user start fill up the page and update wr_pointer when commit is done.
 *   if the page is full then un-map.
 *   start - again
 *
 */
