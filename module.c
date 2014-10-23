/**
 * module.c
 *
 *  Created on: 23 Oct 2014
 *      Author: lester
 *
 */

/**
 * My hello world module
 */

/// kernel headers
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
//#include <linux/wrapper.h>

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
static char *message_pos;        ///<current message send position

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
  if (open_) return -EBUSY;

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
  /* Number of bytes actually written to the buffer */
  int bytes_read = 0;
  /* If we’re at the end of the message, return 0
   * (which signifies end of file) */
  if (*message_pos == 0) return 0;
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
static int Major;

struct file_operations Fops = {
 .read = device_read,
 .write = device_write,
 .open = device_open,
 .release = device_release,
};
/*
 *
 */
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
  printk("you’ll have to create a device file. \n");
  printk("We suggest you use:\n");
  printk("mknod <name> c %d <minor>\n", Major);
  printk("You can try different minor numbers %s", "and see what happens.\n");
  return 0;
}

void char_dev_clean(void)
{
  /* Unregister the device */
  unregister_chrdev(Major, DEVICE_NAME);
  /* If there’s an error, report it */
}

/**
 * Module initialization routine
 * @return non zero means module fail
 */
static int init(void)
{
  printk("Hello, world - this is the kernel speaking\n");
  return char_dev_init();
  return 0;
}

/**
 *  Cleanup - undid whatever init_module did
 */
static void cleanup(void)
{
  printk("Short is the life of a kernel module\n");
  char_dev_clean();
}

module_init(init);
module_exit(cleanup);
