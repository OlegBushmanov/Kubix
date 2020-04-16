/*
 *     kbx_storage.h
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

#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/connector.h>

#ifndef _KBX_STORAGE__H
#define _KBX_STORAGE__H

/* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
 * Hash table for channel nodes
 * --------------------------------------------------------------------------------
 * */
#define CHAN_HT_WIDTH        8
struct kubix_channels{
    DECLARE_HASHTABLE(chan_hash, CHAN_HT_WIDTH);
    int idx_val;                  /* struct cb_id .val running */
};
enum CHAN_NODE_STATE{             /* netlink connector states */
    CHAN_NODE_INIT,               /* UCM bloked Controller, waiting handshake start */
    CHAN_NODE_HANDSHAKE,          /* KCM: sent a handshake request */
    CHAN_NODE_NETLINK,            /* Controller: handshake responsed */
    CHAN_NODE_DESTROY,            /* Socket closed: KCM can reset the context */
};
struct chan_node{
    int               state;      /* Chan Node state */
        /* channel identity */
    pid_t             pid;        /* connection originator's PID */
    int               unique_id;  /* unique value served */
        /* Netlink connector attributes */
    struct cb_id      id;         /* NL Connector callback ID */
    u32               seq;        /* NL Connector message sequence number */
        /* return from user space */
    int               rspmsg_len; /* its length */
    u8               *rspmsg;     /* message to the userspace */
    int               user_ret;   /* save user return in void call */
        /* Hashtable variables */
    wait_queue_head_t rspmsg_q;   /* poll/read wait queue */
    struct mutex      lock;       /* protects struct members */
    struct hlist_node node;       /* hashtable bucket list ref */
};

/* ss_storage.c: functions prototypes*/
int  kubix_store_init(void);
void kubix_store_destroy(void);
#define create_chan_node(x, y, o) add_chan_node(x,  y, 0, o)
int  add_chan_node(pid_t pid, s32 uid, struct cb_id*, /* cb_id for reserved only */
        struct chan_node **chan_node);
int  find_chan_node(pid_t pid, s32 uid, struct chan_node**);
int  del_chan_node(pid_t pid, s32 uid, struct chan_node**);
void show_chan_buckets(void);
void show_chan_bucket(int bkt);
void purify_chan_buckets(void);

const char *str_channel_state(int state);

#endif /* _KBX_STORAGE__H */
