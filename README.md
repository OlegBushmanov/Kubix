<p style="line-height: 0.9em;">
<pre>
 *           Kernel User space Bus for Interprocess eXchange                .
 *                        Protocol Version 0.1
 *
 *          ┌──────────┐
 *          │ User Bus │                    User Space
 *          ├──────────┤
 *          │ Dispatch │>────┬──────────┬──────────┬───── . . . . ────┐
 *          │ messages │     v          v          v                  v
 * ┌───────┐├──────────┤ ┌───────┐  ┌───────┐  ┌───────┐          ┌───────┐ 
 * │ node0 ││ Hash Tbl │ │ node1 │  │ node2 │  │ node3 │          │ nodeN │
 * └───────┘└──────────┘ └───────┘  └───────┘  └───────┘          └───────┘
 *     ^          ^          ^          ^          ^                  ^
 *     ┆        N │          ┆ v        │ v        ┆ v                ┆ v
 *     ┆        e │ P        ┆ i        │ i        ┆ i                ┆ i
 *     ┆        t │ h        ┆ r        │ r        ┆ r                ┆ r
 *     ┆        l │ y        ┆ t        │ t        ┆ t                ┆ t
 *     ┆        i │ z        ┆ u        │ u        ┆ u                ┆ u
 *     ┆        n │          ┆ a        │ a        ┆ a                ┆ a
 *     ┆        k │          ┆ l        │ l        ┆ l                ┆ l
 *     ┆          │          ┆          │          ┆                  ┆
 * ────┆──────────│──────────┆──────────┆──────────┆──────────────────┆────
 *     ┆        C │          ┆          │          ┆                  ┆
 *     ┆        o │ c        ┆ c        │ c        ┆ c                ┆ c
 *     ┆        n │ h        ┆ h        │ h        ┆ h                ┆ h
 *     ┆        n │ a        ┆ a        │ a        ┆ a                ┆ a
 *     ┆        e │ n        ┆ n        │ n        ┆ n                ┆ n
 *     ┆        t │ n        ┆ n        │ n        ┆ n                ┆ n
 *     ┆        t │ e        ┆ e        │ e        ┆ e                ┆ e
 *     ┆        o │ l        ┆ l        │ l        ┆ l                ┆ l
 *     ┆        r │          ┆          │          ┆                  ┆
 *     v          v          v          v          v                  v
 * ┌───────┐┌──────────┐ ┌───────┐  ┌───────┐  ┌───────┐          ┌───────┐ 
 * │ node0 ││ Hash Tbl │ │ node1 │  │ node2 │  │ node3 │          │ nodeN │
 * └───────┘├──────────┤ └───────┘  └───────┘  └───────┘          └───────┘
 *          │ Dispatch │     ^          ^          ^                  ^
 *          │ messages │>────┴──────────┴──────────┴───── . . . . ────┘
 *          ├──────────┤
 *          │ Kernel   │                  Kernel Space
 *          │      Bus │
 *          └──────────┘
 </pre>
 </p>
 KUBUX is a bus for communication between user space processes/threads and
 kernel threads, those are modules or bottom parts of user space processes.<br/>
<br/>
 1. Communication is initiated from the kernel. A kernel thread requests
    a channel between itself and some user space process or thread. Kernel
    thread PID or TID plus some unique (for the thread) value comprise a pair
    which used as a key for hashtables, both in kernel and user spaces.
    Two nodes in opposite hashtables formate a Kubix virtual channel.<br/>
<br/>
 2. A user space application, compiled with Kubix, binds hashtable nodes to
    processes or threads, communicating to kernel partnership threads.
    Related channel nodes in user Kubix stores pid and unique value of
    the kernel adversary in the virtual channel.<br/>
<br/>
 3. Physical data transmission goes through Netlink Connector group. You
    have recompiled Linux kernel to allocate NL connector index. Kubix
    has two data distrubutors: one in kernel space and another in user
    space. A distributer identifies a message come and transfer it to
    the related channel node, (pid, uid) pair is used to perform the action.<br/>
<br/>
 4. Kubix is agnostic to communication payload; it just transfers buffers
    between kernel and user space processes.
    As example kernel thread unique values that can be socket IDs -- TCP or
    files while intercepting open, read, write, close or connect, bind, close
    system calls...<br/>
<br/>
 5. Netlink Connector could be easily replaced by Generic Netlimk family
     socket.<br/>
<br/>
 6. See the diagram above:<br/>

    Kernel bus <──────> User bus   - hysical Netlink channel
    Kernel node <┄┄┄┄┄> Usr node    - virtual Kubix channels
    Kernel node0 <┄┄┄┄> User node0  - communication within Kubix - between
                                      the kernel module and a userspace
                                      application.
<br/>
 7. In this project a socket from the netlink connector family provides physical commucation between
    kernel and userspace, for details of API refer to 
    Linux [kernel connector](https://www.kernel.org/doc/Documentation/connector/connector.txt).
    Recompile linu kernel and install.
    Review [sample](https://elixir.bootlin.com/linux/v4.18/source/samples/connector)
<br/><br/>
 * 2020+ Copyright (c) Oleg Bushmanov <olegbush55@hotmail.com>
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
