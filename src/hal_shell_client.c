/*
 * Copyright (c) 2018 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: hal_shell_client.c
 */


/**
 *       @file  npu_shell.c
 *      @brief  a shell that can interract with the hald
 *
 *   @internal
 *     Created  05/11/2014
 *     Company  DELL
 *
 * =====================================================================================
 */


#include "std_cmd_redir.h"
#include "std_select_tools.h"
#include "std_file_utils.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


int main(int argc, char**argv) {
    char * path = "/var/run/ar.npu.shell";

    int opt = -1;
    char unit_str[30]="";
    char cmd_str[1024]="";
    char *exit_cmd="\n::exit\n";
    char *prompt_off="\n::prompt:off\n";
    char *prompt_on="\n::prompt:on\n";

    while ((opt=getopt(argc,argv,"u:c:"))!=-1) {
        switch(opt) {
        case 'u':
            snprintf(unit_str,sizeof(unit_str)-1,"::%s\n",optarg);
            break;
        case 'c':
            snprintf(cmd_str,sizeof(cmd_str)-1,"\n\n%s\n",optarg);
            break;
        default:
            printf("Valid args are [ -u unit -c \"cmd\" ]\n");
            exit (1);
        }
    }

    int sock=-1;

    if (std_cmd_redir_connect(path,&sock)!=STD_ERR_OK)
        return -1;
    if (strlen(cmd_str)>0) {
        int rc = std_write(sock,prompt_off,strlen(prompt_off)+1,true,NULL);
        if (rc==-1) return -1;
    } else {
        int rc = std_write(sock,prompt_on,strlen(prompt_on)+1,true,NULL);
        if (rc==-1) return -1;
        printf("Welcome to the NPU Shell\ntype ::exit to exit the shell and ::[npu] to change the default npu\n");
    }
    if (strlen(unit_str)>0) {
        int rc = std_write(sock,unit_str,strlen(unit_str)+1,true,NULL);
        if (rc==-1) return -1;
    }
    if (strlen(cmd_str)>0) {
        int rc = std_write(sock,cmd_str,strlen(cmd_str)+1,true,NULL);
        if (rc==-1) return -1;
    }

    while (true) {
        fflush(stdout);
        struct timeval tv={1,0};
        fd_set rset;
        int selfds[]={STDIN_FILENO,sock};
        int max_fd = -1;
        std_sel_adds_set(selfds,sizeof(selfds)/sizeof(*selfds),
                    &rset,&max_fd,true);

         int rc = std_select_ignore_intr(max_fd+1,&rset,NULL,NULL,&tv,NULL);

        if (rc==0) {
            if (strlen(cmd_str)>0) {
                int rc = std_write(sock,exit_cmd,strlen(exit_cmd)+1,true,NULL);
                if (rc==-1) return -1;
             }
             continue;
        }
        if (rc==-1) break;
        if (FD_ISSET(STDIN_FILENO,&rset)) {
            if ((rc=std_fd_copy(sock,STDIN_FILENO,NULL))==-1 || rc==0) break;
        }
        if (FD_ISSET(sock,&rset)) {
            if ((rc=std_fd_copy(STDOUT_FILENO,sock,NULL))==-1 || rc==0) break;
            fflush(stdout);
        }
    }
    return 0;
}

