/*
 *     kbx_storage.c
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

#include <linux/kernel.h>
#include <linux/module.h>    /* Required for EXPORT_SYMBOL */
#include <linux/linkage.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/connector.h>
#include "kubix_main.h"
#include "kbx_storage.h"

#define KUBIX "storage"

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 * Hash table for channel nodes
 * --------------------------------------------------------------------------------
 * */
static DEFINE_SPINLOCK(kubix_ht_lock);
static struct kubix_channels *channels = NULL; /* Channel storage for sessions */
/* --------------------------------------------------------------------------------
 * */
#define get_composite_key(v1, v2) (s64)((((u64)v2) << 32) | (u64)v1)
#define hash_key_fmt "hash_min(pid=%d, uid=%d)=>bucket[%d]"
#define hash_key_val(p, u, k) p, u, hash_min(k, CHAN_HT_WIDTH)
/* --------------------------------------------------------------------------------
 * */
const char *str_channel_state(int state)
{
    switch(state){
        case CHAN_NODE_INIT:      return "CHAN_NODE_INIT";      break;
        case CHAN_NODE_HANDSHAKE: return "CHAN_NODE_HANDSHAKE"; break;
        case CHAN_NODE_NETLINK:   return "CHAN_NODE_NETLINK";   break;
        case CHAN_NODE_DESTROY:   return "CHAN_NODE_DESTROY";   break;
    }
    return "";
}
/* --------------------------------------------------------------------------------
 * */
int kubix_store_init(void)
{
    int ret = 0;

    if(channels){
        printk(KERN_DEBUG KUBIX": %d, %s KUBIX exists, no reinit allowed.\n",
                __LINE__, __func__);
        ret = -1;
        goto out;
    }
    spin_lock(&kubix_ht_lock);
    channels = kzalloc(sizeof(struct kubix_channels), GFP_KERNEL);
    hash_init(channels->chan_hash);
    channels->idx_val = 1;
    spin_unlock(&kubix_ht_lock);

out:
    return ret;
}
EXPORT_SYMBOL(kubix_store_init);

/* --------------------------------------------------------------------------------
 * */
void kubix_store_destroy(void)
{
    if(channels){
        spin_lock(&kubix_ht_lock);
        kfree(channels);
        channels = NULL;
        printk(KERN_DEBUG KUBIX": %d, %s KUBIX destroyed.\n",
                __LINE__, __func__);
        spin_unlock(&kubix_ht_lock);
        goto out;
    }
    printk(KERN_DEBUG KUBIX": %d, %s KUBIX was not initialized.\n",
            __LINE__, __func__);
out:
    return;
}
EXPORT_SYMBOL(kubix_store_destroy);

/* --------------------------------------------------------------------------------
 * */
int add_chan_node(pid_t pid, s32 uid, struct cb_id *id_ptr,
        struct chan_node **chan_node)
{
    int ret = 0;
    struct chan_node *chaninfo;
    s64 key = get_composite_key(pid, uid);
    *chan_node = NULL;

    chaninfo = kzalloc(sizeof(struct chan_node), GFP_KERNEL);
    if(chaninfo == NULL){
        printk(KERN_ALERT KUBIX": %d, memory allocation failed...!\n",
                __LINE__);
        ret = -1;
        goto out;
    }

    chaninfo->seq = 1;
    chaninfo->state = CHAN_NODE_INIT;
    chaninfo->pid = pid;
    chaninfo->unique_id = uid;
    chaninfo->rspmsg = NULL;
    chaninfo->rspmsg_len = 0;
    mutex_init(&chaninfo->lock);
    init_waitqueue_head(&chaninfo->rspmsg_q);

    spin_lock(&kubix_ht_lock);
    if(id_ptr){ /* reset socket context */
        memcpy(&chaninfo->id, id_ptr, sizeof(struct cb_id));
    }
    else{
        chaninfo->id.idx = CN_SS_IDX;
        chaninfo->id.val = CN_SS_VAL;
    }

    hash_add(channels->chan_hash, &chaninfo->node, key);
    spin_unlock(&kubix_ht_lock);

    printk(KERN_DEBUG KUBIX": %d, %s - added channel node "hash_key_fmt"\n",
           __LINE__, __func__, hash_key_val(pid, uid, key));
    *chan_node = chaninfo;

out:
    return ret;
}
EXPORT_SYMBOL(add_chan_node);

/* --------------------------------------------------------------------------------
 * */
int find_chan_node(pid_t pid, s32 uid, struct chan_node **chaninfo_o)
{
    int ret = -1;
    struct chan_node *chaninfo;
    s64 key = get_composite_key(pid, uid);
    *chaninfo_o = NULL;

    printk(KERN_DEBUG KUBIX": %d, %s - seeking for node %d.%d ...\n",
            __LINE__, __func__, pid, uid);
    spin_lock(&kubix_ht_lock);
    hash_for_each_possible(channels->chan_hash, chaninfo, node, key) {
        if(chaninfo->pid == pid && chaninfo->unique_id == uid) {
            printk(KERN_DEBUG KUBIX": %d, %s - found pid %d - uid %d "
                    hash_key_fmt", state '%s' cb_id[%d, 0x%u]\n",
                   __LINE__, __func__, pid, uid,
                  hash_key_val(pid, uid, key), str_channel_state(chaninfo->state),
                  chaninfo->id.idx, chaninfo->id.val);
            *chaninfo_o = chaninfo;
            ret = 0;
            break;
        }
    }
    spin_unlock(&kubix_ht_lock);
    return ret;
}
EXPORT_SYMBOL(find_chan_node);

/* --------------------------------------------------------------------------------
 * */
int del_chan_node(pid_t pid, s32 uid, struct chan_node **chaninfo_o)
{
    int ret = -1;
    struct chan_node *chaninfo;
    s64 key = get_composite_key(pid, uid);
    *chaninfo_o = NULL;
    printk(KERN_DEBUG KUBIX": %d, %s - deleting for node %d.%d ...\n",
            __LINE__, __func__, pid, uid);
    spin_lock(&kubix_ht_lock);
    hash_for_each_possible(channels->chan_hash, chaninfo, node, key) {
        if(chaninfo->pid == pid && chaninfo->unique_id == uid) {
            ret = 0;
            printk(KERN_DEBUG KUBIX": %d, %s - deleted "hash_key_fmt
                   " cb_id[%d, 0x%u]\n",
                   __LINE__, __func__, hash_key_val(pid, uid, key),
                   chaninfo->id.idx, chaninfo->id.val);
            *chaninfo_o = chaninfo;
            hash_del(&chaninfo->node);
        }
    }
    spin_unlock(&kubix_ht_lock);
    return ret;
}
EXPORT_SYMBOL(del_chan_node);

/* --------------------------------------------------------------------------------
 * */
void show_chan_buckets(void)
{
    struct chan_node *obj;
    int i=0;
    printk(KERN_DEBUG KUBIX": %d, %s - Kubix hashtable content(%p)\n",
            __LINE__, __func__, &channels->chan_hash);
    spin_lock(&kubix_ht_lock);
    for (i = 0; i < HASH_SIZE(channels->chan_hash); ++i) {
        if (!hlist_empty(&channels->chan_hash[i])) {
            hlist_for_each_entry(obj, &channels->chan_hash[i], node) {
                printk(KERN_DEBUG KUBIX": bucket[%d] => pid %d - uid %d, "
                       "cbid[%d,0x%u]",
                        i, obj->pid, obj->unique_id, obj->id.idx, obj->id.val);
            }
        }
    }
    spin_unlock(&kubix_ht_lock);
}
EXPORT_SYMBOL(show_chan_buckets);

/* --------------------------------------------------------------------------------
 * */
void show_chan_bucket(int i)
{
    struct chan_node *obj;
    printk(KERN_DEBUG KUBIX": %d, %s - Kubix hashtable bucket %d content(%p)\n",
            __LINE__, __func__, i, &channels->chan_hash);
    spin_lock(&kubix_ht_lock);
    if (!hlist_empty(&channels->chan_hash[i])) {
        hlist_for_each_entry(obj, &channels->chan_hash[i], node) {
            printk(KERN_DEBUG KUBIX": bucket[%d] => pid %d - uid %d, "
                   "cbid[%d,0x%u]",
                    i, obj->pid, obj->unique_id, obj->id.idx, obj->id.val);
        }
    }
    spin_unlock(&kubix_ht_lock);

}
EXPORT_SYMBOL(show_chan_bucket);

/* --------------------------------------------------------------------------------
 * */
void purify_chan_buckets(void)
{
    struct chan_node *obj;
    int i=0;
    spin_lock(&kubix_ht_lock);
    for (i = 0; i < HASH_SIZE(channels->chan_hash); ++i) {
        if (!hlist_empty(&channels->chan_hash[i])) {
            hlist_for_each_entry(obj, &channels->chan_hash[i], node) {
                printk(KERN_DEBUG KUBIX": delete bucket[%d] => pid %d - uid %d, "
                       "cbid[%d,0x%u]",
                        i, obj->pid, obj->unique_id, obj->id.idx, obj->id.val);
                hash_del(&obj->node);
                kfree(obj);
            }
        }
    }
    spin_unlock(&kubix_ht_lock);
}
EXPORT_SYMBOL(purify_chan_buckets);

/* --------------------------------------------------------------------------------
 * */
#if 0
#endif
