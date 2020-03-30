/*
 * 	kbx_channel.h
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

#include "kbx_storage.h"

#ifndef _KBX_CHANNEL__H_
#define _KBX_CHANNEL__H_
/* --------------------------------------------------------------------------------
 * */
enum OPERATION_TYPES{
	KUBIX_CHANNEL,      /*     get kubix channel for transmit    */
	KERNEL_REQUEST,     /*     send data to user & wait back     */
	USER_MESSAGE,       /*     check channel on user data        */
	KERNEL_RELEASE,     /*     notify user on kernel close       */
	USER_RELEASE,       /*     notify kernel on user close       */
	KERNEL_REPORT,      /*     report to kubix user side         */
	NO_ACTION,          /*     remove this from code             */
};
/* --------------------------------------------------------------------------------
 * */
enum OP_OUTCOME{
    KBX_SUCCEESED = 0,
    KBX_IMPOSSIBLE_STATE,
    KBX_IMPOSSIBLE_OP,
};
/* --------------------------------------------------------------------------------
 * */
struct kubix_hdr{
	s32 pid;                /* kernel process id */
	s32 uid;                /* kernel unique id of process resource, like socket */
	u8 opt;                 /* kubix operation type */
	u8 ret;                 /* user kubix return code 0-success, negative-error */
	s16 data_len;           /* payload data len */
	u8  data[0];            /* payload data related to process resource
                             * kubix is agnostic to payload content
                             */
};
/* --------------------------------------------------------------------------------
 * */
struct cm_handshake_result{
	s32 result; /* 0 = success */
	s32 len;
	u8  data[0];
};
/* --------------------------------------------------------------------------------
 * */
void send_message_to_user(struct kubix_hdr *, u32 seq);
void send_kubix_handshake(struct chan_node *chaninfo);
void cn_user_msg_callback(struct cn_msg *msg, struct netlink_skb_parms *nsp);
int  get_verified_channel(pid_t, s32, void*, int, struct chan_node **c);
int  read_user_message_to_kernel(pid_t pid, s32 uid, void **msg, int *len);
int write_kernel_message_to_user(pid_t pid, s32 uid, void*, int len, int op);
/* --------------------------------------------------------------------------------
 * */
enum IC_CALL_TYPES{
	THE_BIND_CALL,
	THE_CONNECT_CALL,
	THE_ACCEPT_CALL,
};
/* --------------------------------------------------------------------------------
 * */
#endif
