#ifndef __KUBIX__H
#define __KUBIX__H

#endif /* __KUBIX__H */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *           Kernel User space Bus for Interprocess eXchange                .
 *                        Protocol Version 0.1
 *
 *          +----------+
 *          | User Bus |                    User Space
 *          +----------+
 *          | Dispatch |>----+----------+----------+----- . . . . ----+
 *          | messages |     v          v          v                  v
 * +-------++----------+ +-------+  +-------+  +-------+          +-------+
 * | node0 || Hash Tbl | | node1 |  | node2 |  | node3 |          | nodeN |
 * +-------++----------+ +-------+  +-------+  +-------+          +-------+
 *     ^      ^      ^       ^          ^          ^                  ^
 *     .      | N    |       . v        . v        . v                . v
 *     .      | e  P |       . i        . i        . i                . i
 *     .      | t  h |       . r        . r        . r                . r
 *     .      | l  y |       . t        . t        . t                . t
 *     .      | i  z |       . u        . u        . u                . u
 *     .      | n    |       . a        . a        . a                . a
 *     .      | k    |       . l        . l        . l                . l
 *     .      |      |       .          .          .                  .
 * ____.______|______|_______.__________,__________.__________________.____
 *     .      | C    |       .          .          .
 *     .      | o  C |       . c        . c        . c                . c
 *     .      | n  h |       . h        . h        . h                . h
 *     .      | n  a |       . a        . a        . a                . a
 *     .      | e  n |       . n        . n        . n                . n
 *     .      | t  n |       . n        . n        . n                . n
 *     .      | t  e |       . e        . e        . e                . e
 *     .      | o  l |       . l        . l        . l                . l
 *     .      | r    |       .          .          .                  .
 *     v      v      v       v          v          v                  v
 * +-------++----------+ +-------+  +-------+  +-------+          +-------+
 * | node0 || Hash Tbl | | node1 |  | node2 |  | node3 |          | nodeN |
 * +-------++----------+ +-------+  +-------+  +-------+          +-------+
 *          | Dispatch |     ^          ^          ^                  ^
 *          | messages |>----+----------+----------+----- . . . . ----+
 *          +----------+
 *          | Kernel   |                  Kernel Space
 *          |      Bus |
 *          +----------+
 *
 * KUBUX is a bus for communication between user space processes/threads and
 * kernel threads, those are modules or bottom parts of user space processes.
 *
 * 1. Communication is initiated from the kernel. A kernel thread requests
 *    a channel between itself and some user space process or thread. Kernel
 *    thread PID or TID plus some unique (for the thread) value comprise a pair
 *    which used as a key for hashtables, both in kernel and user spaces.
 *    Two nodes in opposite hashtables formate a Kubix virtual channel.
 *
 * 2. A user space application, compiled with Kubix, binds hashtable nodes to
 *    processes or threads, communicating to kernel partnership threads.
 *    Related channel nodes in user Kubix stores pid and unique value of
 *    the kernel adversary in the virtual channel.
 *
 * 3. Physical data transmission goes through Netlink Connector group. You
 *    have recompiled Linux kernel to allocate NL connector index. Kubix
 *    has two data distrubutors: one in kernel space and another in user
 *    space. A distributer identifies a came message and transfer it to
 *    the related channel node, (pid, uid) is used to perform the action.
 *
 * 4. Kubix is agnostic to communication payload; it just transfers buffers
 *    between kernel and user space processes.
 *    As example kernel thread unique values can be socket IDs -- TCP or
 *    files while intercepting open, read, write, close or connect, bind, close
 *    system calls...
 *
 * 5. Netlink Connector could be easily replace by Generic Netlimk family
 *    socket.
 *
 * 6. See the diagram above:
 *
 *    Kernel bus <------>  User bus   - hysical Netlink channel
 *    Kernel node <.....> Usr node    - virtual Kubix channels
 *    Kernel Node0 <....> User Node0  - communication with Application itself.
 *
 */
