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
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/connector.h>
#include "kubix_main.h"
#include "kbx_storage.h"
#include "kbx_channel.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Oleg Bushmanov");
MODULE_DESCRIPTION("Kernel to User Bus for Interprocess Transmissions");
MODULE_VERSION("1.0");
#define KUBIX "kubix.module: "

static struct cb_id kubix_id = { CN_SS_IDX, CN_SS_VAL };
static char kubix_name[] = "kubix";
static pid_t kubix_pid =   0;
static s32 kubix_uid   = -10;
static struct sock *nls;

/* ------------------------------------------------------------------------------
 */
static int kubix_init(void)
{
	int err;
    struct chan_node *kubix_node;

	printk(KERN_INFO KUBIX"... init started \n");

	err = cn_add_callback(&kubix_id, kubix_name, cn_user_msg_callback);
	if(err){
		printk(KERN_ERR KUBIX" faield to register CN callback.\n");
		goto err_out;
	}

	err = kubix_store_init();
	if(err){
		printk(KERN_ERR KUBIX" faield to initialize kubix_channels storage.\n");
		goto err_out;
	}

	err = create_chan_node(kubix_pid, kubix_uid, &kubix_node);
    if(err < 0)
		return err;

	printk(KERN_INFO KUBIX"initialized kubix_channels with id={%u.%u}\n",
		kubix_id.idx, kubix_id.val);

	send_kubix_handshake(kubix_node);

	return 0;

err_out:
	if (nls && nls->sk_socket)
		sock_release(nls->sk_socket);

	return err;
}

/* ------------------------------------------------------------------------------
 */
static void kubix_fini(void)
{
	cn_del_callback(&kubix_id);
	if (nls && nls->sk_socket)
		sock_release(nls->sk_socket);

	kubix_store_destroy();

	printk(KERN_INFO KUBIX"fini stopped ... \n");
}

module_init(kubix_init);
module_exit(kubix_fini);

/* ------------------------------------------------------------------------------
 */
