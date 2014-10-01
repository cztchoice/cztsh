#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include "czt.h"
#include <signal.h>
#include <time.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>


#ifndef MAX_LINE
#define MAX_LINE 1000
#endif

#ifndef ARG_MAX
#define ARG_MAX 100
#endif

#ifndef BUILTIN_COMMAND_NUM
#define BUILTIN_COMMAND_NUM 100
#endif

static sigjmp_buf sh_signal_jmpbuf;
pid_t shell_pgrp = 0;
int shell_tty = 0;

int getTokens(char *line, char **tokens){
    if(NULL == line || NULL == tokens){
        return -1;
    }
    bool word_begin = false;
    while(*line != '\0'){
        if(isspace(*line)){
            *line = '\0';
            line++;
            if(word_begin){
                word_begin = false;
            }
        }
        else{
            if(!word_begin){
                word_begin = true;
                *tokens = line;
                tokens++;
            }
            line++;
        }
    }
    //Here must be * tokens
    *tokens = NULL;
    return 0;
}

static char *BuiltinCommand[ BUILTIN_COMMAND_NUM ] = {
    "", //address start in 1
    "alias",
    "bind",
    "builtin",
    "caller",
    "command",
    "declare",
    "echo",
    "enable",
    "help",
    "let",
    "local",
    "logout",
    "mapfile",
    "printf",
    "read",
    "readarray",
    "source",
    "type",
    "typeset",
    "ulimit",
    "unalias",
    "" //The end of BuiltinCommands
};

int isBuiltinCommand(char *str){
    int i = 1;
    while(strcmp(BuiltinCommand[i], str) != 0){
        if(strcmp(BuiltinCommand[i], "") == 0){
            return 0;
        }
        i++;
    }
    return i;
}

int handleBuiltin(char **tokens){
    printf("builtin handling....");
    return 0;
}
int handleExec(char **tokens){
    // char **it = tokens;
    // while(*it){
    //     printf("%s\n", *it);
    //     it++;
    // }
    pid_t pid;
    int status;
    if((pid = fork()) < 0){
        err_sys("fork error");
        return -1;
    }
    else if(0 == pid){

        if(execvp(*tokens, tokens) < 0){
            err_sys("execv error");
        }
    }
    else{
        printf("child pid: %d\n", pid);
        waitpid(pid, &status, 0);
    }
    return 0;
}

void sh_sig_handler(int signum, siginfo_t *info, void *context)
{
    printf("%s\n", strsignal(signum));
    int status;
    if (signum == SIGCHLD)
    {
        if(info->si_errno != 0){
            fprintf(stderr, "SIG_CHLD error: %s\n", strerror(info->si_errno));
        }
        switch (info->si_code){
        case CLD_STOPPED:
            printf("SIGCHLD, si_pid: %d, si_status: %d, si_uid: %d stopped\n", info->si_pid, info->si_status, info->si_uid);
            break;
        case CLD_CONTINUED:
            //here we need to set shell to continued wait for child process
            //出现问题：我waitpid之后 在vim里的响应依然很慢，感觉有另一个进程在和它争夺控制权一样
            tcsetprgp(STDIN_FILENO, )
            waitpid(info->si_pid, &status, 0);

            // printf("SIGCHLD, si_pid: %d, si_status: %d, si_uid: %d continue\n", info->si_pid, info->si_status, info->si_uid);
            break;
        case CLD_EXITED:
            printf("SIGCHLD, si_pid: %d, si_status: %d, si_uid: %d exited\n", info->si_pid, info->si_status, info->si_uid);
            break;
        default:
            //si_code = CLD_EXITED, CLD_KILLED, CLD_DUMPED, CLD_TRAPPED
            printf("Received SIGCHLD, si_pid: %d, si_status: %d, si_uid: %d\n", info->si_pid, info->si_status, info->si_uid);
            break;
        }  
        
    }
    else if (signum == SIGINT)
    {   
        //printf("%d\n",signum);        
        printf("Received SIGINT, exiting ... \n");
    }
    else if (signum == SIGTSTP)
    {   
        //printf("%d\n",signum);        
        printf("Received SIGTSTP, exiting ... \n");
    }
    else
    {
        time_t mytime = time(0);
        printf("%d: %s\n", signum, asctime(localtime(&mytime)));
        printf("%d\n",signum);
    }
    siglongjmp(sh_signal_jmpbuf, 1);
}

int main(int argc, char const *argv[])
{
    shell_pgrp = getpgid (0);
    shell_tty = dup (fileno (stderr));

    if(shell_pgrp == -1){
        err_sys("getpgid error!");
    }

    if(shell_pgrp == 0){
        shell_pgrp = getpid();
        setpgid(0, shell_pgrp);
        tcsetprgp(shell_tty, shell_pgrp);
    }

    


    //Set shell signal action
    struct sigaction new_action, old_action;
    // action.sa_handler = sh_sig_handler;
    new_action.sa_sigaction = sh_sig_handler;
    sigemptyset(&(new_action.sa_mask));
    new_action.sa_flags = 0;
    new_action.sa_flags |= SA_SIGINFO;
    //TODO: 继续思索需要处理哪些信号
    sigaction(SIGTERM, &new_action, &old_action);
    sigaction(SIGQUIT, &new_action, &old_action);
    sigaction(SIGINT, &new_action, &old_action);

    //怎么控制下面两个信号，从而是的vim等程序在Ctrl+Z 停止时，得到正确的结果？参见apue p-283
    //这是 job control的内容，需要自己再了解下
    //http://stackoverflow.com/questions/11821378/what-does-bashno-job-control-in-this-shell-mean
    //http://www.opensource.apple.com/source/bash/bash-80/bash/jobs.c?c
    sigaction(SIGTSTP, &new_action, &old_action);
    sigaction(SIGCHLD, &new_action, &old_action);

    //This code is from
    //https://github.com/perusio/linux-programming-by-example/blob/master/book/ch10/ch10-status.c
    //
    // sigset_t childset;
    // sigemptyset(& childset);
    // sigaddset(& childset, SIGCHLD);
    // sigprocmask(SIG_SETMASK, & childset, NULL);


    FILE *fp = stdin;
    char *line = (char *)malloc(MAX_LINE + 1);
    size_t len = 0;
    ssize_t read;
    char *tokens[ARG_MAX];
    while (true) {
        if(sigsetjmp(sh_signal_jmpbuf, 1)){
            printf("Continue from signal...\n");
        }
        if((read = getline(&line, &len, fp)) == -1){
            err_quit("getline error, shell stop.");
        }
        if(getTokens(line, tokens) < 0){
            err_ret("tokens parse error!");
        }

        if(isBuiltinCommand(tokens[0])){
            handleBuiltin(tokens);
        }
        else{
            handleExec(tokens);
        }
    }

    free(line);
    return 0;
}
