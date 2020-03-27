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
#define KUBIX "kubix.module."

static struct cb_id kubix_id = { CN_SS_IDX, CN_SS_VAL };
static char kubix_name[] = "kubix";
static pid_t kubix_pid =   0;
static s32 kubix_uid   = -10;
static struct sock *nls;

/* ------------------------------------------------------------------------------
 * this function is declared but not exported! so repeat it here
 */
int cn_cb_equal(struct cb_id *i1, struct cb_id *i2)
{
	return ((i1->idx == i2->idx) && (i1->val == i2->val));
}

/* ------------------------------------------------------------------------------
 */
static int kubix_init(void)
{
	int err;
    struct chan_node *kubix_node;

	err = cn_add_callback(&kubix_id, kubix_name, cn_user_msg_callback);
	if (err)
		goto err_out;

	printk(KERN_INFO KUBIX"initialized with id={%u.%u}\n",
		kubix_id.idx, kubix_id.val);

	err = create_chan_node(kubix_pid, kubix_uid, &kubix_node);
    if(err < 0)
		return err;

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
}

module_init(kubix_init);
module_exit(kubix_fini);

/* ------------------------------------------------------------------------------
 */
