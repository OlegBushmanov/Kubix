/*
 * 	bus_test.cpp
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

#define UNIT_TEST
#include "../kubix.h"

int userTestLogic(UserCallbackCtx *ctx)
{
	fprintf(stderr, "%d %s: processing kernel message on %d.%d "
			" length %d.\n",
			__LINE__, __func__, ctx->pid, ctx->uid, ctx->len);
	ctx->ret = 0;
	return 0;
}

int main()
{
	Kubix bus;
	Node *node;
	struct cb_id cbid = { .idx = CN_SS_IDX, .val = CN_SS_VAL, };
	for(int i = -10;i < 0;i++){
		bus.createNode(0, i, node);
	}
	bus.dump();
	for(int i = -8;i < 0;i += 2){
		if(!bus.eraseNode(0, i, node)){
			fprintf(stderr, "failed to delete node for key(0, %d)\n", i);
			continue;
		}
		fprintf(stderr, "deleted node[%d.%d], id[%d,%d], state '%s', "
				"buff[0x%u], len %d, type %d, result %d\n",
				node->_pid,
				node->_unique,
				strNodeState(node->_state), node->_recv_buffer,
				node->_recv_len, node->_opt , node->_ret);
	}
	bus.dump();
	bus._user_app_callback = &userTestLogic;

	char buffer[PAYLOAD_MAX_SIZE];
	int len;
	char hello_01[] = "Hello message 01";
	char hello_02[] = "Hello message 02";
	char hello_03[] = "Hello message 03";
	char hello_04[] = "Hello message 04";
	char hello_05[] = "Hello message 05";
	int op, ret, err;

	fprintf(stderr, "** write into a Bus node\n");
	bus.putMsg(0, -9, hello_01, sizeof(hello_01));
	bus.getMessage(0, -9, op, ret, &buffer, len);
	fprintf(stderr, "** got from bus '%s'\n", buffer);

	fprintf(stderr, "** missing node\n");
	bus.getMessage(0, -8, op, ret, &buffer, len);

	pthread_t tid = bus.runBus();

	while(1){
		// wait for kernel message
		if(!bus.getMessage(0, -10, op, ret, &buffer, len))
			continue;
		// send response back to kernal with user app payload instead.
		err = bus.send2kernel( 0, -10, 0, 0, hello_03, sizeof(hello_03));
	}

    pthread_join(tid, NULL);

	return 0;
}
