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
#include <paths.h>
#include <termios.h>
#include <assert.h>


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


int shell_tty = 0;
int sh_id;


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
    "",
    "alias",
    "bind",
    "builtin",
    "caller",
    "command",
    "declare",
    "echo",
    "enable",
    "fg",
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
    ""//The end of BuiltinCommands
};

int isBuiltinCommand(char *str){
    int i = 1;
    while(strcmp(BuiltinCommand[i], "") != 0){
        if(strcmp(BuiltinCommand[i], str) == 0){
            return i;
        }
        i++;
    }
    return 0;
}

int handleBuiltin(char **tokens){
    printf("In Handle Builtin...\n");
    int status;
    if(strcmp(tokens[0], "fg") == 0){
        //Don't deal with process in signal handler funtion!
        int pid = atoi(tokens[1]);
        int pgid = getpgid(pid);
        printf("pid: %d, pgid: %d\n", pid, pgid);
        killpg(pgid, SIGCONT);
        tcsetpgrp (shell_tty, pgid);
        
        waitpid (pid, &status, 0);
    }
    printf("builtin handling....\n");
    return 0;
}
int handleExec(char **tokens){
    // char **it = tokens;
    // while(*it){
    //     printf("%s\n", *it);
    //     it++;
    // }
    printf("In Handle Exec...\n");
    pid_t pid;
    pid_t pgid;
    int status;
    if((pid = fork()) < 0){
        err_sys("fork error");
        return -1;
    }
    else if(0 == pid){
        pgid = getpid();
        printf("%d\n", pgid);
        if (setpgid(pgid, pgid) < 0){
            err_sys("setpgid error");
        }
        printf("pid: %d, pgid: %d, pgrp: %d\n", getpid(), getpgid(0), getpgrp());
        if (tcsetpgrp(shell_tty, pgid) < 0){
            err_sys("tcsetpgrp error");
        }

        signal (SIGINT, SIG_DFL);
        signal (SIGQUIT, SIG_DFL);
        signal (SIGTSTP, SIG_DFL);
        signal (SIGTTIN, SIG_DFL);
        signal (SIGTTOU, SIG_DFL);
        signal (SIGCHLD, SIG_DFL);

        if(execvp(*tokens, tokens) < 0){
            err_quit("execv error");
        }

    }
    else{
        setpgid(pid, pid);
        tcsetpgrp(shell_tty, pid);

        printf("child pid: %d\n", pid);
        waitpid(pid, &status, 0);

        tcsetpgrp(shell_tty, getpid());
    }
    return 0;
}

void print_info(char *str, siginfo_t *info)
{
    printf("SIGCHLD %s, si_pid: %d, si_status: %d, si_uid: %d stopped\n", str, info->si_pid, info->si_status, info->si_uid);
}

void sh_sig_handler(int signum, siginfo_t *info, void *context)
{
    printf("%s\n", strsignal(signum));
    if (signum == SIGINT)
    {   
        //printf("%d\n",signum);        
        printf("Received SIGINT, ignore ... \n");
    }
    else if (signum == SIGTSTP)
    {   
        //printf("%d\n",signum);        
        printf("Received SIGTSTP, ignore ... \n");
    }
    else if (signum == SIGTTIN){
        // printf("Received SIGTTIN, ignore ... \n");
    }
    else if (signum == SIGTTOU){
        // printf("Received SIGTTOU, ignore ... \n");
    }
    else
    {
        time_t mytime = time(0);
        printf("%d: %s\n", signum, asctime(localtime(&mytime)));
        printf("%d\n",signum);
    }
    siglongjmp(sh_signal_jmpbuf, 1);
}

void sh_sig_chld_handler(int signum, siginfo_t *info, void *context)
{
    //chld handler 不需要跳转？
    printf("%s\n", strsignal(signum));
    // int status;
    assert (signum == SIGCHLD);
    if(info->si_errno != 0){
        fprintf(stderr, "SIG_CHLD error: %s\n", strerror(info->si_errno));
    }
    switch (info->si_code){
    case CLD_STOPPED:
        // if(0 == sh_back_grp){
        //     sh_back_grp = info->si_pid;
        // }
        //setpgid(0, getpid());
        //tcsetpgrp(shell_tty, getpid());
        print_info("CLD_STOPPED", info);
        break;
    case CLD_CONTINUED:
        //here we need to set shell to continued wait for child process
        //出现问题：我waitpid之后 在vim里的响应依然很慢，感觉有另一个进程在和它争夺控制权一样
        //不应该在signal handler里进行handle？
        // setpgid(info->si_pid, info->si_pid);
        // tcsetpgrp(shell_tty, info->si_pid);
        // killpg(info->si_pid, SIGCONT);
        // tcsetprgp(STDIN_FILENO, )
        //waitpid(info->si_pid, &status, 0);
        
        print_info("CLD_CONTINUED", info);
        break;
    case CLD_EXITED:
        print_info("CLD_EXITED", info);
        break;
    case CLD_TRAPPED:
        print_info("CLD_TRAPPED", info);
        break;
    case CLD_KILLED:
        print_info("CLD_KILLED", info);
        break;
    case CLD_DUMPED:
        print_info("CLD_DUMPED", info);
        break;
    default:
        //si_code = CLD_EXITED, CLD_KILLED, CLD_DUMPED, CLD_TRAPPED
        printf("%d\n", info->si_code);
        print_info("SIGCHLD Others", info);
        break;
    }
}

int main(int argc, char const *argv[])
{
    sh_id = getpgid (0);
    if(sh_id == -1){
        err_sys("getpgid error!");
    }

    shell_tty = STDIN_FILENO;
    
    if(setpgid(sh_id, sh_id) < 0){
        err_sys("setpgid error");
    }
    tcsetpgrp(shell_tty, sh_id); 

    printf("pid: %d, pgid: %d\n", getpid(), getpgrp());

    // Set shell signal action
    struct sigaction new_action, old_action, chld_action;
    // action.sa_handler = sh_sig_handler;
    new_action.sa_sigaction = sh_sig_handler;
    sigemptyset(&(new_action.sa_mask));
    new_action.sa_flags = 0;
    new_action.sa_flags |= SA_SIGINFO;
    //TODO: 继续思索需要处理哪些信号
    // sigaction(SIGTERM, &new_action, &old_action);
    sigaction(SIGQUIT, &new_action, &old_action);
    sigaction(SIGINT, &new_action, &old_action);

    //怎么控制下面两个信号，从而是的vim等程序在Ctrl+Z 停止时，得到正确的结果？参见apue p-283
    //这是 job control的内容，需要自己再了解下
    //http://stackoverflow.com/questions/11821378/what-does-bashno-job-control-in-this-shell-mean
    //http://www.opensource.apple.com/source/bash/bash-80/bash/jobs.c?c
    
    sigaction(SIGTSTP, &new_action, &old_action);
    

    //when shell is in the background process group
    // sigaction(SIGTTIN, &new_action, &old_action);
    // sigaction(SIGTTOU, &new_action, &old_action);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    chld_action.sa_sigaction = sh_sig_chld_handler;
    sigemptyset(&(chld_action.sa_mask));
    chld_action.sa_flags = 0;
    chld_action.sa_flags |= SA_SIGINFO;
    sigaction(SIGCHLD, &chld_action, NULL);

    // This code is from
    // https://github.com/perusio/linux-programming-by-example/blob/master/book/ch10/ch10-status.c
    // block the CHLD signal
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
            if(handleBuiltin(tokens) == -1){
                err_sys("handleBuiltin error");
            }
        }
        else{
            if( handleExec(tokens) == -1){
                err_sys("handleExec error");
            }
        }
    }

    free(line);
    return 0;
}
