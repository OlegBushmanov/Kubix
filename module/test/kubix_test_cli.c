/*
 * 	kubix_test_cli.c
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

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include "test_command.h"

static struct __attribute__((aligned( 32 ))) { 
    struct __attribute__((__packed__)) {
        struct test_command cmd;
        char buf[ DATA_BUFFER_LEN ];
    };
} scmd, receive;

static int     cmd_exchange_size;
static pid_t   pid = 3124;
static int     uid =    12;

void print_prompt()
{
    printf("Choose command by type: \n"
           "1 for OPEN_CHANNEL,\n"
           "2 for CLOSE_CHANNEL,\n"
           "3 for SEND_COMMAND,\n"
           "4 for SEND_RESULT,\n"
           "0 for quii\n");
}

int proc_input(int fd, int ct)
{
    int ret = 0;

    switch(ct){
    case OPEN_CHANNEL:
        sprintf(scmd.buf, "open channel for [%d,%d]", pid, uid);
        scmd.cmd.len = strlen(scmd.buf) + 1;
        scmd.cmd.type = ct;
        scmd.cmd.pid  = pid;
        scmd.cmd.uid  = uid;
    break;
    case CLOSE_CHANNEL: break;
        sprintf(scmd.buf, "close channel for [%d,%d]", pid, uid);
        scmd.cmd.len = strlen(scmd.buf) + 1;
        scmd.cmd.type = ct;
        scmd.cmd.pid  = pid;
        scmd.cmd.uid  = uid;
    case SEND_COMMAND: break;
    case SEND_RESULT: break;
        return ret;
    }

    printf("Writing message to the device [%s].\n", scmd.buf);
    ret = write(fd, &scmd, ECXG_MSG_LENGTH(scmd));
    if(ret < 0){
        perror("Failed to write the message to the device.");
        return errno;
    }

    printf("Press ENTER to read back from the device...\n");
    getchar();

    printf("Reading from the device...\n");
    ret = read(fd, &receive, sizeof(receive));
    if(ret < 0){
        perror("Failed to read the message from the device.");
        return errno;
    }

    return ret;
}
int main()
{
    int err, fd, type = -1;
    printf("Starting device test code example...\n");
    fd = open("/dev/kbxchar", O_RDWR);
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    // printf("Type in a short string to send to the kernel module:\n");
    // scanf("%[^\n]%*c", stringToSend);
    print_prompt();
    do{
        scanf("%d", &type);
        if(type < 1){
            printf("Stop testing...\n");
            break;
        }
        err = proc_input(fd, type);
        if(!err){
            printf("Error in driver: %d\n", err);
            return err;
        }
        printf("The received message is: [%s]\n", receive.buf);
    } while(1);

    printf("End of the program\n");
    return 0;
}

