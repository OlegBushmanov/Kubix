#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/timer.h>

#include <linux/connector.h>

#include "kbx_channel.h"

#define KUBIX "channel"

/* -----------------------------------------------------------------------------
 * */
static int get_user_message(struct chan_node*, pid_t, s32, void **, int *len);
/* -----------------------------------------------------------------------------
 * */
static struct cb_id kubix_id = { CN_SS_IDX, CN_SS_VAL };
static int kbx_pid =   0;
static int kbx_uid = -10;
/* -----------------------------------------------------------------------------
 * */
const char *str_ops_type(int type)
{
	switch(type){
		case KUBIX_CHANNEL: return "KUBIX_CHANNEL"; break;
		case KERNEL_REQUEST:  return "KERNEL_REQUEST";  break;
		case USER_MESSAGE: return "USER_MESSAGE"; break;
		case KERNEL_RELEASE: return "KERNEL_RELEASE"; break;
		case USER_RELEASE: return "USER_RELEASE"; break;
		case KERNEL_REPORT: return "KERNEL_REPORT"; break;
		case NO_ACTION:  return "NO_ACTION";  break;
	}
	return "";
}
/* -----------------------------------------------------------------------------
 * */
void send_message_to_user(struct kubix_hdr *data, u32 seq)
{
	struct cn_msg *m;
	int len = sizeof(*data) + data->data_len;

	printk(KERN_DEBUG KUBIX": %d,%s - [%d,%d] seq  %u, type %s\n",
			__LINE__, __func__,
			data->pid, data->uid, seq, str_ops_type(data->opt));

	m = kzalloc(sizeof(*m) + len + 1, GFP_ATOMIC);

    m->id.idx = CN_SS_IDX;
    m->id.val = CN_SS_VAL;

	m->len = len;
	memcpy(m + 1, data, m->len);

    m->ack =   0;
	m->seq = seq;

	cn_netlink_send(m, 0, CN_SS_IDX, GFP_ATOMIC);
	kfree(m);
}
/* -----------------------------------------------------------------------------
 * */
void send_kubix_handshake(struct chan_node *chaninfo)
{
	char hello[] = "Hello from kernel, Controller";
	struct kubix_hdr *hndshk;
	int len = sizeof(hello);
	hndshk = kzalloc(sizeof(*hndshk) + len + 1, GFP_ATOMIC);
	memcpy(&hndshk->data, hello, len);
	hndshk->data_len = len;
	send_message_to_user(hndshk, chaninfo->seq++);
	return;
}
/* -----------------------------------------------------------------------------
 * */
static void kubix_handshake(struct chan_node *kbxinfo, struct kubix_hdr *rsp)
{
	struct cm_handshake_result *result;
	if(rsp->opt == CHAN_NODE_HANDSHAKE){/* CM response on initial handshake */
		result = (struct cm_handshake_result*)rsp->data;
		if(result->result < 0 ){
			printk(KERN_ERR KUBIX": %d, %s - handshake %d.%d: user rejection\n",
					__LINE__, __func__, rsp->pid, rsp->uid);
			/* ignore -- chaninfo->state = CHAN_NODE_DESTROY;
             * close Kubix 
             * print error
             * unload module --  sys_delete_module(name, flags)
             * */
		}
		kbxinfo->state = CHAN_NODE_NETLINK;
		/* print handshake message */
        return;
	}
    printk(KERN_INFO KUBIX": %d, %s - handshake %d.%d was not requested.\n",
           __LINE__, __func__, rsp->pid, rsp->uid);
}
/* -----------------------------------------------------------------------------
 * Netlink connector callback for use responses
 * */
void cn_user_msg_callback(struct cn_msg *msg, struct netlink_skb_parms *nsp)
{
	struct chan_node *chaninfo = NULL;
	int len = msg->len;
	struct kubix_hdr *kbx_hdr;

    /* if the message come is unwanted guest
     */
	if(!cn_cb_equal(&msg->id, &kubix_id)){
		printk(KERN_INFO KUBIX": %d, %s - spurious message %d.%d <> %d.%d\n",
			   __LINE__, __func__, msg->id.idx, msg->id.val,
			   kubix_id.idx, kubix_id.val);
		goto out;
	}

	printk(KERN_DEBUG KUBIX": %d, %s - %lu idx=%x, val=%x, seq=%u, ack=%u, "
			"len=%d: %p.\n",
	        __LINE__, __func__, jiffies, msg->id.idx, msg->id.val,
	        msg->seq, msg->ack, msg->len,
	        msg->len ? (char *)msg->data : "");

	/* check if a body of the user controller response does exist
	 */
	kbx_hdr = 0 < len ? (struct kubix_hdr *)msg->data : NULL;
	if(!kbx_hdr){
		printk(KERN_ERR KUBIX": %d, %s -  empty netlink message!\n",
				__LINE__, __func__);
		goto wake_up_out;
	}

	/*--------------------------------------------------------------------------
     * find a channel node related to the response
	 */
	if(find_chan_node(kbx_hdr->pid, kbx_hdr->uid, &chaninfo) < 0){
		printk(KERN_DEBUG KUBIX": %d, %s -  [%d.%d] node does not exists\n",
				__LINE__, __func__, kbx_hdr->pid, kbx_hdr->uid);
		goto wake_up_out;
	}
	printk(KERN_INFO KUBIX": %d, %s - Operation in user message %s; "
		   "Related chanmel node [%d.%d] in state %s; resulted as %d\n",
		   __LINE__, __func__, str_ops_type(kbx_hdr->opt), kbx_hdr->pid,
		   kbx_hdr->uid, str_channel_state(chaninfo->state), kbx_hdr->ret);

	/*..........................................................................
	 *  now response processing
	 */
	mutex_lock(&chaninfo->lock);
	/* Catch start KUBIX initialization ++++++++++++++++++++++++++++++++++++++++
     * zero process and negative unique value relate to the main channel
	 * between kerneland user controllers themselves:
     * kbx_pid =   0
     * kbx_uid = -10
     * only one case at startup time - unlikely
	 */
	if(unlikely(chaninfo->pid == kbx_pid && chaninfo->unique_id == kbx_uid)){
		printk(KERN_INFO KUBIX": %d, %s - %d.%d is KUBIX main node, "
								"type %s\n",
						 __LINE__, __func__, chaninfo->pid, chaninfo->unique_id,
						 str_ops_type(kbx_hdr->opt));
		/* response processing for KUBIX
		 */
		kubix_handshake(chaninfo, kbx_hdr);
		goto unlock_out;
	}
    /*--------------------------------------------------------------------------
     * compare an operation type in kubix message and used channel state
     * something like FST.
     */
	switch(chaninfo->state){
		case CHAN_NODE_INIT:
			/* print impossible state */
			kbx_hdr->ret = -(KBX_IMPOSSIBLE_STATE);
			break;
		case CHAN_NODE_HANDSHAKE: // process
			if(kbx_hdr->opt != KUBIX_CHANNEL)
				kbx_hdr->ret = -(KBX_IMPOSSIBLE_OP);
			else if(!kbx_hdr->ret)
				chaninfo->state = CHAN_NODE_NETLINK;
			break;
		case CHAN_NODE_NETLINK: // already opened channel
			switch(kbx_hdr->opt) {
			case USER_MESSAGE: break;
			case USER_RELEASE: chaninfo->state = CHAN_NODE_DESTROY; break;
			case KUBIX_CHANNEL:
			case KERNEL_REQUEST:
			case KERNEL_RELEASE:
			case KERNEL_REPORT:
			case NO_ACTION:
				kbx_hdr->ret = -(KBX_IMPOSSIBLE_OP);
				goto unlock_out;
            }
		case CHAN_NODE_DESTROY:
			del_chan_node(kbx_hdr->pid, kbx_hdr->uid, &chaninfo);
			kfree(chaninfo);
			goto unlock_out;
	}
	chaninfo->rspmsg = kmalloc(kbx_hdr->data_len, GFP_KERNEL);
	chaninfo->rspmsg_len = kbx_hdr->data_len;
	memcpy(chaninfo->rspmsg, kbx_hdr->data, kbx_hdr->data_len);
	chaninfo->user_ret = kbx_hdr->ret;

unlock_out:
	mutex_unlock(&chaninfo->lock);
wake_up_out:
	wake_up_interruptible(&chaninfo->rspmsg_q);
out:
	return;
}
/* -----------------------------------------------------------------------------
 * */
// static
int get_user_message(struct chan_node *chaninfo, pid_t pid, s32 uid,
				            void **rsp, int *len)
{
	int ret = -1;
	*rsp = NULL;
	*len = 0;

	if(chaninfo->pid != pid || chaninfo->unique_id != uid){
		printk(KERN_DEBUG KUBIX": %d, %s - info[%d,%d] != response[%d,%d]\n",
			__LINE__, __func__, chaninfo->pid, chaninfo->unique_id,
			pid, uid);
		goto out;
	}

	/* wait until user message come
	 */
	if(!chaninfo->rspmsg_len)
		wait_event_interruptible(chaninfo->rspmsg_q, chaninfo->rspmsg_len > 0);
	/* transfer user message to a caller and nulify the channel buffer
	 */
	*len = chaninfo->rspmsg_len;
	*rsp = chaninfo->rspmsg;
	ret  = chaninfo->user_ret;

	chaninfo->rspmsg_len = 0;
	chaninfo->rspmsg = NULL;

out:
	return ret;
}
/* ----------------------------------------------------------------------------- */
int get_verified_channel(pid_t pid, s32 uid, void *msg, int len,
                         struct chan_node **chan)
{
	u32 seq;
	struct chan_node *chaninfo;
	struct kubix_hdr *req;
	int ret = -1;
	*chan = NULL;

	printk(KERN_INFO KUBIX": %d, %s - verified channel for [%d.%d]\n",
			__LINE__, __func__, pid, uid);
	/*  */
	if(find_chan_node(pid, uid, &chaninfo) < 0){
		printk(KERN_DEBUG KUBIX": %d, %s - pid %d-uid %d node is not found\n",
				__LINE__, __func__, pid, uid);
		if(create_chan_node(pid, uid, &chaninfo) < 0){
			printk(KERN_ERR KUBIX": %d, %s - cannot allocate node %d.%d\n",
				   __LINE__, __func__, pid, uid);
			goto out;
		}
		*chan = chaninfo; /* a new node */
	}
    printk(KERN_INFO KUBIX": %d, %s -  [%d.%d] node status %s\n",
           __LINE__, __func__, pid, uid, str_channel_state(chaninfo->state));

	switch(chaninfo->state){
	case CHAN_NODE_INIT:
		if(*chan == NULL){
			printk(KERN_ERR KUBIX": %d, %s - node %d.%d exists as pending."
				   " by another call\n",
				   __LINE__, __func__, pid, uid);
			ret = -2;
			goto out;
		}
		break;
	case CHAN_NODE_HANDSHAKE:
		printk(KERN_INFO KUBIX": %d, %s - node %d.%d is passing another init\n",
			   __LINE__, __func__, pid, uid);
		ret = -3;
		goto out;
	case CHAN_NODE_NETLINK:
		printk(KERN_INFO KUBIX": %d, %s - node %d.%d is active \n",
			   __LINE__, __func__,
			   pid, uid);
		*chan = chaninfo;
		ret = 0;
		goto out;
    case CHAN_NODE_DESTROY:
		printk(KERN_INFO KUBIX": %d, %s - pre-existed node schedule for remove\n",
			    __LINE__, __func__);
		del_chan_node(pid, uid, &chaninfo);
		kfree(chaninfo);
		goto unlock_out;
	}
	/* transfer request to user space */
	req = kzalloc(sizeof(*req) + len, GFP_ATOMIC);
	req->pid = pid;
	req->uid = uid;
	req->opt = KUBIX_CHANNEL;
	req->data_len = len;
	memcpy(req->data, msg, len);
	seq = chaninfo->seq++;

	send_message_to_user(req, seq);
    chaninfo->state = CHAN_NODE_HANDSHAKE;

	kfree(req);

	wait_event_interruptible(chaninfo->rspmsg_q, chaninfo->rspmsg_len > 0);
	ret = chaninfo->user_ret;
	goto out;

unlock_out:
	(void)0;
out:
	return ret;
}
EXPORT_SYMBOL(get_verified_channel);
/* ----------------------------------------------------------------------------- */
int read_user_message_to_kernel(pid_t pid, s32 uid, void **msg, int *len)
{
	int ret = -1;
	struct chan_node *chaninfo;

	printk(KERN_INFO KUBIX": %d, %s - reading from node %d.%d\n",
			__LINE__, __func__, pid, uid);
	if(find_chan_node(pid, uid, &chaninfo) < 0){
		printk(KERN_DEBUG KUBIX": %d, %s - node %d.%d is not found\n",
				__LINE__, __func__, pid, uid);
		goto out;
	}
	/* move "mutex_lock(&chaninfo->lock);" here */
	if(chaninfo->state != CHAN_NODE_NETLINK){
		printk(KERN_ERR KUBIX": %d, %s - for uid = %d %s \n",
				__LINE__, __func__, uid,
				"NL connector is not ready or closed");
		goto out;
	}
	ret = get_user_message(chaninfo, pid, uid, msg, len);

out:
	/* move "mutex_unlock(&chaninfo->lock);" here, a label before */
	return ret;
}
EXPORT_SYMBOL(read_user_message_to_kernel);
/* ----------------------------------------------------------------------------- */
int write_kernel_message_to_user(pid_t pid, s32 uid, void *msg, int len, int op)
{
	int ret = -1;
	u32 seq;
	struct chan_node *chaninfo;
	struct kubix_hdr *req = NULL;

	printk(KERN_INFO KUBIX": %d, %s - sending from node %d.%d\n",
			__LINE__, __func__, pid, uid);
	/* move "mutex_lock(&chaninfo->lock);" here */
	if(find_chan_node(pid, uid, &chaninfo) < 0){
		printk(KERN_DEBUG KUBIX": %d, %s - node %d.%d is not found\n",
				__LINE__, __func__, pid, uid);
		goto out;
	}
    if(op != KERNEL_REQUEST && op != KERNEL_RELEASE && op != KERNEL_REPORT){
        ret = -(KBX_IMPOSSIBLE_OP);
		goto out;
    }

	if(chaninfo->state != CHAN_NODE_NETLINK){
		printk(KERN_ERR KUBIX": %d, %s - for uid = %d %s \n",
				__LINE__, __func__, uid,
				"NL connector is not ready or closed");
		goto out;
	}
	req = kzalloc(sizeof(*req) + len + 1, GFP_KERNEL);
    req->pid = pid;
    req->uid = uid;
    req->opt = op; /* KERNEL_REQUEST || KERNEL_RELEASE || KERNEL_REPORT */
    req->ret = 0;
	memcpy(req->data, msg, len);
    req->data_len = len;

	seq = chaninfo->seq++;
	send_message_to_user(req, seq);
    if(op == KERNEL_REQUEST)
		ret = get_user_message(chaninfo, pid, uid, &msg, &len);
											/*      msg,  len ignored */
    if(op == KERNEL_RELEASE)
		chaninfo->state = CHAN_NODE_DESTROY;

out:
	/* move "mutex_unlock(&chaninfo->lock);" here */
	if(req) kfree(req);
	return ret;
}
EXPORT_SYMBOL(write_kernel_message_to_user);
/* -----------------------------------------------------------------------------
 * *
const char *str_req_call(int type)
{
	switch(type){
		case THE_BIND_CALL:    return "THE_BIND_CALL";    break;
		case THE_CONNECT_CALL: return "THE_CONNECT_CALL"; break;
		case THE_ACCEPT_CALL:  return "THE_ACCEPT_CALL";  break;
	}
	return "";
}
 * -----------------------------------------------------------------------------
 * */
