/*
 * 	kubix_tester.c.c
 * 
 * 2020+ Copyright (c) Oleg Bushmanov <olegbush55@hotmai.com>
 * All rights reserved.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "../kbx_channel.h"
#include "test_command.h"

#define  DEVICE_NAME "kbxchar"
#define  CLASS_NAME  "kbx"
#define  KBXTD       "kubix.test.driver: "


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Bushmanov");
MODULE_DESCRIPTION("KBXTD test harness for Google test.");
MODULE_VERSION("0.1");

static int     major_number;
static short   size_of_message;
static int     open_count = 0;

static struct  class  *char_dev_class  = NULL;
static struct  device *char_device = NULL;

static int     test_dev_open(struct inode *, struct file *);
static int     test_dev_release(struct inode *, struct file *);
static ssize_t test_dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t test_dev_write(struct file *, const char *, size_t, loff_t *);

static struct __attribute__((aligned( 64 ))) { 
    struct __attribute__((__packed__)) {
        struct test_command cmd;
        char buf[ DATA_BUFFER_LEN ];
    };
} cmd_exchange;

static int     cmd_exchange_size;
static int     proc_command(void);

static struct file_operations fops = {
       .open    = test_dev_open,     /* open char device*/
       .write   = test_dev_write,    /* receive from user */
       .read    = test_dev_read,     /* send to user */
       .release = test_dev_release,  /* close char device */
    };

static int __init kbxchar_init(void)
{
    int err = 0;

    printk(KERN_INFO KBXTD"Initializing the KBXChar LKM\n");

    /* The major number for the device */
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if(major_number < 0){
        printk(KERN_ALERT "KBXChar failed to register a major number\n");
        err = major_number;
        goto out;
    }
    printk(KERN_INFO KBXTD"registered correctly with major number %d\n",
           major_number);

    /* Register the device class */
    char_dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(char_dev_class)){
        printk(KERN_ALERT "Failed to register device class\n");
        err = PTR_ERR(char_dev_class);
        goto dev_out;
    }
    printk(KERN_INFO KBXTD"device class registered correctly\n");

    /* Register the device driver */
    char_device = device_create(char_dev_class, NULL, MKDEV(major_number, 0),
                                 NULL, DEVICE_NAME);
    if(IS_ERR(char_device)){
        printk(KERN_ALERT "Failed to create the device\n");
        err = PTR_ERR(char_device);
        goto class_out;
    }
    printk(KERN_INFO KBXTD"device class created correctly\n");

    memset(&cmd_exchange, 0x00, sizeof(cmd_exchange));

out:
    return err;

class_out:
    class_destroy(char_dev_class);

dev_out:
    unregister_chrdev(major_number, DEVICE_NAME);

    return err;
}

/** @brief The LKM cleanup function
 *         Similar to the initialization function, it is static. The __exit macro notifies
 *         that if this code is used for a built-in driver (not a LKM) that this function
 *         is not required.
 */
static void __exit kbxchar_exit(void)
{
   device_destroy(char_dev_class, MKDEV(major_number, 0));     // remove the device
   class_unregister(char_dev_class);                          // unregister the device class
   class_destroy(char_dev_class);                             // remove the device class
   unregister_chrdev(major_number, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO KBXTD"Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep  A pointer to a file object (defined in linux/fs.h)
 */
static int test_dev_open(struct inode *inodep, struct file *filep)
{
   open_count++;
   printk(KERN_INFO KBXTD"Device has been opened %d time(s)\n", open_count);
   return 0;
}

/** @brief This function is called whenever device is being read from user space i.e.
 *         data is being sent from the device to the user. In this case is uses the
 *         copy_to_user() function to send the buffer string to the user and captures any
 *         errors.
 *
 *  @param filep   A pointer to a file object (defined in linux/fs.h)
 *  @param buffer  The pointer to the buffer to which this function writes the data
 *  @param len     The length of the b
 *  @param offset  The offset if required
 */
static ssize_t test_dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   size_of_message = sizeof( cmd_exchange.cmd ) + cmd_exchange.cmd.len;
   error_count = copy_to_user(buffer, &cmd_exchange, size_of_message);
   if(error_count == 0){            // if true then have success
      printk(KERN_INFO KBXTD"Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else{
      printk(KERN_INFO KBXTD"Failed to send %d characters to the user\n",
             error_count);
      return -EFAULT;
   }
}

/** @brief This function is called whenever the device is being written to from user
 *         space i.e. data is sent to the device from the user. 
 *
 *  @param filep  A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len    The length of the array of data that is being passed in the const char
 *                buffer
 *  @param offset The offset if required
 */
static ssize_t test_dev_write(struct file *filep, const char *buffer, size_t len,
                         loff_t *offset)
{
    printk(KERN_INFO KBXTD" %d %s - Received %zu characters from the user\n",
          __LINE__, __func__, len);
    memcpy(&cmd_exchange, buffer, len);
    cmd_exchange_size = len;
    proc_command();

    return len;
}

static int proc_command(void)
{
    int err = 0;
    struct chan_node *node;
    int type  = cmd_exchange.cmd.type;
    pid_t pid = cmd_exchange.cmd.pid;
    int uid   = cmd_exchange.cmd.uid;
    char *buf = cmd_exchange.buf;
    int len   = cmd_exchange.cmd.len;

    printk(KERN_INFO KBXTD" %d, %s - type %d, [%d,%d] length %d buf %s\n", 
           __LINE__, __func__, type, pid, uid, cmd_exchange.cmd.len, buf);
    switch( type ){
    case OPEN_CHANNEL:
        {
            err = get_verified_channel(pid, uid, buf, &len, &node);
            if(err){
                printk(KERN_ERR KBXTD" %s, %d - failed to obtain channel for[%d,%d]\n",
                       __func__, __LINE__, pid, uid);
                /* prepare error return to user */
                sprintf(buf, "error: %d - failed to obtain channel.", err);
                cmd_exchange.cmd.len = strlen(buf) + 1;
                goto out;
            }
            printk(KERN_INFO KBXTD" %s, %d - successfully obtained channel for[%d,%d]\n"
                   "use command \n>$ rmmod kubix_channel",
                   __func__, __LINE__, pid, uid);
                /* prepare successful return to user */
            sprintf(buf, "success: obtained channel for[%d,%d]\n", pid, uid);
            cmd_exchange.cmd.len = strlen(buf) + 1;
            goto out;
        }
        break;
    case CLOSE_CHANNEL: break;
    case SEND_COMMAND: break;
    case SEND_RESULT: break;
    }

out:
    return err;
}
/** @brief The device release function that is called whenever the device is
 *         closed/released by the userspace program
 *
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int test_dev_release(struct inode *inodep, struct file *filep)
{
   printk(KERN_INFO KBXTD"Device successfully closed\n");
   return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(kbxchar_init);
module_exit(kbxchar_exit);

