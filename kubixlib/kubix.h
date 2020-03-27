#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <connector.h>
#include <pthread.h>
#include <time.h>

#ifndef KUBIX_H
#define KUBIX_H

#define PAYLOAD_MAX_SIZE	1024
/* ------------------------------------------------------------------------------ */
struct kubix_hdr{
	__s32 pid;              /* kernel process id */
	__s32 uid;              /* kernel unique id of process resource, like socket */
	__u8  opt;              /* kubix operation type */
	__u8  ret;              /* user kubix return code 0-success, negative-error */
	__s16 data_len;         /* payload data len */
	__u8  data[0];          /* payload data related to process resource
                             * kubix is agnostic to payload content
                             */
};

enum OPERATION_TYPES{
	KUBIX_CHANNEL,      /*     get kubix channel for transmit    */
	KERNEL_REQUEST,     /*     send data to user & wait back     */
	USER_MESSAGE,       /*     check channel on user data        */
	KERNEL_RELEASE,     /*     notify user on kernel close       */
	USER_RELEASE,       /*     notify kernel on user close       */
	KERNEL_REPORT,      /*     report to kubix user side         */
	NO_ACTION,          /*     remove this from code             */
};
/* ------------------------------------------------------------------------------ */
class Node{
public:
	Node(int pid, int uid)
	{
        _pid = pid;
		_unique = uid;
		_mutex = PTHREAD_MUTEX_INITIALIZER;
		_cond = PTHREAD_COND_INITIALIZER; // default attributes
	}
	~Node()
	{
		pthread_cond_destroy(&_cond);
		pthread_mutex_destroy(&_mutex);
	}

	class setSignalLock{
        public:
			setSignalLock(pthread_mutex_t*, pthread_cond_t*);
			~setSignalLock();
		private:
			pthread_mutex_t *_mutex_ptr;
			pthread_cond_t *_cond_ptr;
	};
	class setWaitLock{
		public:
			setWaitLock(pthread_mutex_t*, pthread_cond_t*);
			~setWaitLock();
			void waitMsg();
		private:
			pthread_mutex_t *_mutex_ptr;
			pthread_cond_t *_cond_ptr;
			struct timespec _ts;
	};

	pthread_mutex_t _mutex;
	pthread_cond_t _cond;

	enum {
		NLC_NETLINK,
		NLC_DESTROY,
	} state;

    int  _pid;
	int  _unique;
	__u8 _opt;
	__u8 _ret;
	int  _recv_len;
	char _recv_buffer[PAYLOAD_MAX_SIZE];
};
inline const char *strNodeState(int state)
{
	switch(state)
	{
		case Node::NLC_NETLINK: return "NLC_NETLINK";
		case Node::NLC_DESTROY: return "NLC_DESTROY";
	}
	return "N/A";
}

/* ------------------------------------------------------------------------------ */
#define BUS_HT_BITS			12
class Kubix{
public:
	Kubix();
	~Kubix();

    //---------------------------------------------------------------------------
    /* @brief  - creates a Node object in the kubix hashtable, the userspace end
     * of a chanel
     * @parm1 pid  - the id of the kernelspace process/thread
     * @parm2 uid  - the unique value in the kernelspace process/thread
     * @parm3 node - the reference to the pointer of ctreated Node object
     * @return     - 'true' if succeeded, otherwise 'false'.
     */
	bool createNode(int pid, int uid, Node *(&node));

    /* @brief  - finds a Node object in the kubix hashtable by pid and uid
     *           of kernelspace process/thread
     * @parm1 pid  - the id of the kernelspace process/thread
     * @parm2 uid  - the unique value in the kernelspace process/thread
     * @parm3 node - the reference to the pointer of the found Node object
     * @return     - 'true' if succeeded, otherwise 'false'.
     */
	bool findNode(int pid, int uid, Node *(&node));

    /* @brief  - removes a Node object from  the kubix hashtable by pid and uid
     * @parm1 pid  - the id of the kernelspace process/thread
     * @parm2 uid  - the unique value in the kernelspace process/thread
     * @parm3 node - the reference to the pointer of the removed Node object
     * @return     - 'true' if succeeded, otherwise 'false'.
     */
	bool eraseNode(int pid, int uid, Node *(&node));

    /* @brief  - prints content of the kubix hashtable
     */
	void dumpKubix();

    /* @brief  - purifies content of the kubix hashtable
     */
	void purifyKubix();

    //---------------------------------------------------------------------------
	class setKubixLock{
		public:
			setKubixLock(pthread_mutex_t *mp);
			~setKubixLock();
		private:
			pthread_mutex_t *_mutex_ptr;
	};
    //---------------------------------------------------------------------------
	pthread_t runBus();

    //---------------------------------------------------------------------------
    /* @brief  - consider to move logic into C-tor and delete
     */
	int setCnFd();

    /* @brief  - the base method for forming macros implementing different
     *           sends to kernelspace
     * @parm1 pid  - the id of the kernelspace process/thread
     * @parm2 uid  - the unique value in the kernelspace process/thread
     * @parm3 op   - user kubix send types from OPERATION_TYPES enum;
     *               accepted by kernel: KUBIX_CHANNEL, USER_MESSAGE,
     *               USER_RELEASE
     * @parm4 ret  - if a message is a response on KERNEL_REQUEST, indicates
     *               return code: 0-success, -(n)-error of 'n' code
     * @parm5 payload - the pointer to the buffer with user message
     * @parm6 len  - the length of the message in the buffer
     * @return     - 0 if succeeded to send.
     */
	int send2kernel(int pid, int uid, int op, int ret, void *payload, int len);

    /* @brief  - the blocking method for reading messages sent from kernelspace
     * @parm1 pid  - the id of the kernelspace process/thread
     * @parm2 uid  - the unique value in the kernelspace process/thread
     * @parm3 op   - reference on kernel kubix send types from OPERATION_TYPES enum;
     *               relevant are: KUBIX_CHANNEL, KERNEL_REQUEST, KERNEL_RELEASE
     *               KERNEL_REPORT
     * @parm4 ret  - a return reference to the kernel response on USER_MESSAGE
     *               containing requst; return code: 0-success, -(n)-error
     *               of 'n' code
     * @parm5 msg -  the pointer to the buffer with kernel message
     * @parm6 len  - the pointer to thelength of the message in the buffer
     * @return     - false
     */
	bool getMessage(int pid, int uid, int &op, int &ret,
                    char (*msg)[PAYLOAD_MAX_SIZE], int &len);

protected:
    //---------------------------------------------------------------------------
    /* @brief  - consider to move in .cpp
     */
	struct DistributorContext{
		Kubix *_bus;
		int running;
	};
    //---------------------------------------------------------------------------
    /* @brief  - the message polling thread function for pthread_create(...)
     * @parm   - a ponter to the thransmitted thread context
     * @return - the void pointer to the thread return code.
     */
	static void *dispatch(void* context);

private:
	struct pollfd _pfd;
	pthread_mutex_t _bus_mutex;
	std::unordered_map<int, Node*> _nodes;

#ifdef UNIT_TEST
public:
	void putMsg(int pid, int uid, char *msg, int len, bool wakeup = true);
#endif // UNIT_TEST
};

/* ------------------------------------------------------------------------------ */


#endif
