#define UNIT_TEST
#include "../kubix.h"
int main()
{
	Kubix bus;
	Node *node;
	struct cb_id cbid = { .idx = CN_SS_IDX, .val = CN_SS_VAL, };
	for(int i = -10;i < 0;i++){
		bus.createNode(0, i, node);
	}
	bus.dumpKubix();
	for(int i = -10;i < 0;i += 2){
		bus.eraseNode(0, i, node);
		fprintf(stderr, "deleted node[%d.%d], id[%d,%d], state '%s', "
                "buff[0x%u], len %d, type %d, result %d\n",
                node->_pid,
				node->_unique,
			   	strNodeState(node->state), node->_recv_buffer,
			   	node->_recv_len, node->_opt , node->_ret);
	}
	bus.dumpKubix();

	char buffer[PAYLOAD_MAX_SIZE];
   	int len; 
	char hello_01[] = "Hello message 01";
	char hello_02[] = "Hello message 02";
	char hello_03[] = "Hello message 03";
	char hello_04[] = "Hello message 04";
	char hello_05[] = "Hello message 05";

	fprintf(stderr, "** write into a Bus node\n");
	bus.putMsg(0, -9, hello_01, sizeof(hello_01));
	bus.getMessage(0, -9, &buffer, len);
	fprintf(stderr, "** got from bus '%s'\n", buffer);

	fprintf(stderr, "** missing node\n");
	bus.getMessage(0, -10, &buffer, len);

	bus.runBus();

#if 1
	fprintf(stderr, "** test blocking\n");
	bus.getMessage(0, -9, &buffer, len);
#endif
	return 0;
}
