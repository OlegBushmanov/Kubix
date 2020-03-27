#include <iostream>
#include <errno.h>
#include <linux/netlink.h>
#include "kubix.h"

/* ------------------------------------------------------------------------------ */
char *strNodeAttributes(Node *node, char (*arr_ptr)[32])
{
	char *arr = *arr_ptr;
	sprintf(arr,"Node[%d,%d], %s", node->_pid, node->_unique,
			strNodeState(node->state));
	return arr;
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
Node::setSignalLock::setSignalLock(pthread_mutex_t *mp, pthread_cond_t *ep)
	: _mutex_ptr(mp)
	, _cond_ptr(ep)
{
	pthread_mutex_lock(_mutex_ptr);
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
Node::setSignalLock::~setSignalLock()
{
	pthread_cond_signal(_cond_ptr);
	pthread_mutex_unlock(_mutex_ptr);
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
Node::setWaitLock::setWaitLock(pthread_mutex_t *mp, pthread_cond_t *ep)
	: _mutex_ptr(mp)
	, _cond_ptr(ep)
{
	pthread_mutex_lock(_mutex_ptr);
    clock_gettime(CLOCK_REALTIME, &_ts);
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
Node::setWaitLock::~setWaitLock()
{
	pthread_mutex_unlock(_mutex_ptr);
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
void Node::setWaitLock::waitMsg()
{
    _ts.tv_sec += 1;
	pthread_cond_timedwait(_cond_ptr, _mutex_ptr, &_ts);
}
/* ------------------------------------------------------------------------------ */
#define get_composite_key(v1, v2) (int64_t)((((uint64_t)v2) << 32) || (uint64_t)v1)
/* ------------------------------------------------------------------------------ */
Kubix::Kubix()
	: _nodes(1 << BUS_HT_BITS)
{
	setCnFd();
	_bus_mutex = PTHREAD_MUTEX_INITIALIZER;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
Kubix::~Kubix()
{
	purifyKubix();
	pthread_mutex_destroy(&_bus_mutex);
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
bool Kubix::createNode(int pid, int uid, Node *(&node))
{
	Node *node_ptr = nullptr;
	if(findNode(pid, uid, node_ptr)){
		if(node_ptr->NLC_DESTROY)
			delete node_ptr;
		else{
			fprintf(stderr, "%d, %s: Node for uid = %d still in use\n",
					__LINE__, __func__, uid);
			return false;
		}
	}
	setKubixLock lock(&_bus_mutex);
	node_ptr = new Node(pid, uid);
	int64_t key = get_composite_key(pid, uid);
	_nodes[key] = node_ptr;
	fprintf(stderr, "%d, %s: key-key = %ld added Node[Ox%lu]: hash Value [%u],"\
			" bucket [%u]\n",
			__LINE__, __func__, key, node_ptr,
			(_nodes.hash_function)()(key),
			_nodes.bucket(key));
	return true;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
bool Kubix::findNode(int pid, int uid, Node *(&node))
{
	int64_t key = get_composite_key(pid, uid);
	node = nullptr;
	setKubixLock lock(&_bus_mutex);
	auto it = _nodes.find(key);
	if(it == end(_nodes))
		return false;
	node =(*it).second;
	return true;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
bool Kubix::eraseNode(int pid, int uid, Node *(&node))
{
	int64_t key = get_composite_key(pid, uid);
	node = nullptr;
	if(findNode(pid, uid, node)){
		_nodes.erase(key);
		return true;
	}
	return false;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
void Kubix::dumpKubix()
{
	setKubixLock lock(&_bus_mutex);
	char arr[32];
	std::cerr << "_nodes's buckets contain:\n";
	for ( unsigned i = 0; i < _nodes.bucket_count(); ++i) {
		if(0 < _nodes.bucket_size(i)){ 
			std::cerr << "bucket #" << i << " contains:";
			for ( auto local_it  = _nodes.begin(i);
			           local_it!= _nodes.end(i);
			         ++local_it )
				std::cerr << " " << local_it->first << ": "
					<< strNodeAttributes(local_it->second, &arr);
			std::cerr << std::endl;
		}
	}

}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
void Kubix::purifyKubix()
{
	setKubixLock lock(&_bus_mutex);
	for(auto it = _nodes.begin();it != _nodes.end();it++){
		delete it->second;	
	}
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
Kubix::setKubixLock::setKubixLock(pthread_mutex_t *mp)
	: _mutex_ptr(mp)
{
	pthread_mutex_lock(_mutex_ptr);
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
Kubix::setKubixLock::~setKubixLock()
{
	pthread_mutex_unlock(_mutex_ptr);
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
void * Kubix::dispatch(void* context)
{
	struct __attribute__((aligned(NLMSG_ALIGNTO))) {
		struct nlmsghdr nl_hdr;
		struct __attribute__((__packed__)) {
			struct cn_msg cn_msg;
			struct kubix_hdr kbx_msg;
			char buf[1024];
		};
	} rmsg;
    int len;
	time_t tm;
	struct DistributorContext *pctx = (struct DistributorContext*)context;
	Kubix *bus = pctx->_bus;

	pthread_detach(pthread_self());

	fprintf(stderr,"%d:%s:: going to Bus on CN connector fd %d\n",
			__LINE__, __func__, bus->_pfd.fd);

	while(pctx->running){

		switch( poll(&bus->_pfd, 1, -1)) {
			case 0:
				usleep(100);
				continue;
			/*	need_exit break; */
			case -1:
				if (errno != EINTR) {
					fprintf(stderr,"%d:%s:: case -1, errno [%s] %d\n",
							__LINE__, __func__, strerror(errno), errno);
					pctx->running = 0;
				}
				continue;
		}

		memset(&rmsg, 0, sizeof(rmsg));
		len = recv(bus->_pfd.fd, &rmsg, sizeof(rmsg), 0);
		if(len == -1){
			fprintf(stderr,"%d:%s:: after 'recv' call errno '%s' [%d]\n",
					__LINE__, __func__, strerror(errno), errno);
			close(bus->_pfd.fd);
			pctx->running = 0;
			continue;
		}
		switch (rmsg.nl_hdr.nlmsg_type) {
		case NLMSG_ERROR:
			fprintf(stderr, "Error message received 'NLMSG_ERROR'.\n");
			break;
		case NLMSG_DONE:
			time(&tm);
			fprintf(stderr, "%.24s: id[%x.%x] [seq:%u.ack:%u], "
                    "payload[len:%d,0x%u]\n",
				ctime(&tm), rmsg.cn_msg.id.idx, rmsg.cn_msg.id.val, rmsg.cn_msg.seq,
				rmsg.cn_msg.ack, rmsg.cn_msg.len, rmsg.cn_msg.data);
			{
				fprintf(stderr, "payload: node[%d.%d], Kubix msg[len:%d,0x%u],"
                        " type %d\n",
						rmsg.kbx_msg.pid, rmsg.kbx_msg.uid, rmsg.kbx_msg.data_len,
                        rmsg.kbx_msg.data, rmsg.kbx_msg.opt);
				Node *node;
				if(!bus->findNode(rmsg.kbx_msg.pid, rmsg.kbx_msg.uid, node)){
					fprintf(stderr, "lost buffer: node[%d.%d] is not served!\n",
							rmsg.kbx_msg.pid, rmsg.kbx_msg.uid);
					break;
				}
				Node::setSignalLock lock(&node->_mutex, &node->_cond);
				if(0 < node->_recv_len)
					fprintf(stderr, "previous data loss %d\n", node->_recv_len);
				if(rmsg.kbx_msg.data_len){
					memset(node->_recv_buffer, 0x00, PAYLOAD_MAX_SIZE);
					memcpy(node->_recv_buffer, rmsg.kbx_msg.data,
                           rmsg.kbx_msg.data_len);
					node->_recv_len = rmsg.kbx_msg.data_len;
				}
                node->_pid = rmsg.kbx_msg.pid;
                node->_unique = rmsg.kbx_msg.uid;
                node->_opt = rmsg.kbx_msg.opt;
                node->_ret = rmsg.kbx_msg.ret;
			}
			break;
		default:
			break;
		}
	}
	fprintf(stderr,"%d:%s:: quit Bus Loop errno '%s' [%d]\n",
			__LINE__, __func__, strerror(errno), errno);

	return (void*)0; 
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
pthread_t Kubix::runBus()
{
	pthread_t tid;
	struct DistributorContext context = { ._bus = this, .running = 1, };
	pthread_create( &tid, NULL, &Kubix::dispatch, &context);

	return tid;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
int Kubix::setCnFd()
{
	struct sockaddr_nl l_local;

	_pfd.fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
	if(_pfd.fd == -1) {
		perror("socket");
		return -1;
	}

	l_local.nl_family = AF_NETLINK;
	l_local.nl_groups = CN_SS_IDX; /* bitmask of requested groups */
	l_local.nl_pid = 0;

	fprintf(stderr, "subscribing to %u.%u\n", CN_SS_IDX, CN_SS_VAL);

	if(bind(_pfd.fd, (struct sockaddr *)&l_local, sizeof(struct sockaddr_nl)) < 0 ){
		perror("bind");
		close(_pfd.fd);
		return -1;
	}

	_pfd.events = POLLIN;
	_pfd.revents = 0;

    return _pfd.fd;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
int Kubix::send2kernel(int pid, int uid, int op, int ret, void *payload, int len)
{
	int fd = _pfd.fd;
	struct __attribute__((aligned(NLMSG_ALIGNTO))) {
		struct nlmsghdr nl_hdr;
		struct __attribute__((__packed__)) {
			struct cn_msg cn_msg;
			struct kubix_hdr kbx_msg;
			char buf[1024];
		};
	} smsg;
	int cn_msg_data_len = sizeof(struct kubix_hdr) + len;
	int nlmsg_data_len  = NLMSG_LENGTH(sizeof(struct cn_msg) + cn_msg_data_len);
	int smsg_len        = sizeof(struct nlmsghdr) + nlmsg_data_len;

	memset(&smsg, 0, sizeof(smsg));
	smsg.nl_hdr.nlmsg_len = nlmsg_data_len;         /* Netlink */
	smsg.nl_hdr.nlmsg_pid = getpid();
	smsg.nl_hdr.nlmsg_type = NLMSG_DONE;
	smsg.cn_msg.id.idx = CN_SS_IDX;                 /* Connector */
	smsg.cn_msg.id.val = CN_SS_VAL;
    smsg.cn_msg.ack = 0;
    smsg.cn_msg.len = 0;
	smsg.cn_msg.len = cn_msg_data_len;
	smsg.kbx_msg.pid = pid;                         /* Kubix */
	smsg.kbx_msg.uid = uid;
	smsg.kbx_msg.opt = op;
	smsg.kbx_msg.ret = ret;
	smsg.kbx_msg.data_len = len;
	memcpy(&smsg.buf, payload, len);                /* App payload */
	if(send(fd, &smsg, smsg_len, 0) != smsg_len){
		perror("send");
		return -1;
	}
	return 0;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
bool Kubix::getMessage(int pid, int uid, int &op, int &ret,
                       char (*buffer)[PAYLOAD_MAX_SIZE], int &len)
{
	Node *node;
	if(!findNode(pid, uid, node)){
		fprintf(stderr, "%d:%s: no related node[%d.%d] object in Kubix\n",
			   __LINE__, __func__, pid, uid);
		return false;
	}
	Node::setWaitLock lock(&node->_mutex, &node->_cond);

	while(!node->_recv_len)
		lock.waitMsg();

	char *msg = *buffer;
	memset(msg, 0x00, sizeof(buffer));

	memcpy(msg, node->_recv_buffer, node->_recv_len);
	len = node->_recv_len;
    op  = node->_opt;
    ret = node->_ret;

	memset(node->_recv_buffer, 0x00, sizeof(node->_recv_buffer));
	node->_recv_len = 0;

	return false;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

#ifdef UNIT_TEST
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
void Kubix::putMsg(int pid, int uid, char *msg, int len, bool wake_up)
{
	Node *node;
	if(!findNode(pid, uid, node)){
		fprintf(stderr, "%d:%s: not related node[%d.%d] object in Kubix\n",
			   __LINE__, __func__, pid, uid);
		return;
	}
	memset(node->_recv_buffer, 0x00, sizeof(node->_recv_buffer));
	memcpy(node->_recv_buffer, msg, len);
	node->_recv_len = len;
	fprintf(stderr, "%d:%s: message [%s] length %d in the Kubix, key[%d.%d]\n",
			__LINE__, __func__, msg, len, pid, uid);
	if(wake_up)
		pthread_cond_signal(&node->_cond);
	return;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */
#endif // UNIT_TEST
