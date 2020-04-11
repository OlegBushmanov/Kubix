/*
 * 	kubix_main.c
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

/* ------------------------------------------------------------------------------
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>	/* Required for EXPORT_SYMBOL */
#include <linux/linkage.h>
#include <linux/errno.h>
#include "../kbx_channel.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Bushmanov");
MODULE_DESCRIPTION("Kernel to User Bus for Interprocess Transmissions");
MODULE_VERSION("1.0");
#define KUBIX "kubix.test: "


/* ------------------------------------------------------------------------------
 */
static int kubix_init(void)
{
	int err, len;
    struct chan_node *node;
    pid_t pid;
    void *buf;
    buf = kzalloc(1024, GFP_KERNEL);
    len = sizeof("test get_verified_channel");
    memcpy(buf, "test get_verified_channel", len);


    pid = current->pid;
    err = 0;

	printk(KERN_INFO KUBIX"... init started \n");

	err = get_verified_channel(pid, 1, buf, &len, &node);
	if(err){
		printk(KERN_ERR KUBIX" %s, %d - faild to obtain Kubux channel for[%d,1]\n",
               __func__, __LINE__, pid);
		goto err_out;
	}


err_out:
	return err;
}

/* ------------------------------------------------------------------------------
 */
static void kubix_fini(void)
{
	printk(KERN_INFO KUBIX"fini stopped ... \n");
}

module_init(kubix_init);
module_exit(kubix_fini);

/* ------------------------------------------------------------------------------
 */
