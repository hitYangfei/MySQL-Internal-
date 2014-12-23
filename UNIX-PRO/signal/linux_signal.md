## SIGNAL 处理

### 信号默认处理行为

Each signal has a current disposition, which determines how the process behaves when it is delivered the signal.

>*The entries in the "Action" column of the tables below specify the default disposition for each signal, as follows:
>*Term   Default action is to terminate the process.
>*Ign    Default action is to ignore the signal.
>*Core   Default action is to terminate the process and dump core (see core(5)).
>*Stop   Default action is to stop the process.
>*Cont   Default action is to continue the process if it is currently stopped.

### 标准信号

>      Signal     Value     Action   Comment
       ──────────────────────────────────────────────────────────────────────
       SIGHUP        1       Term    Hangup detected on controlling terminal
                                     or death of controlling process
       SIGINT        2       Term    Interrupt from keyboard
       SIGQUIT       3       Core    Quit from keyboard
       SIGILL        4       Core    Illegal Instruction
       SIGABRT       6       Core    Abort signal from abort(3)
       SIGFPE        8       Core    Floating point exception
       SIGKILL       9       Term    Kill signal
       SIGSEGV      11       Core    Invalid memory reference
       SIGPIPE      13       Term    Broken pipe: write to pipe with no
                                     readers
       SIGALRM      14       Term    Timer signal from alarm(2)
       SIGTERM      15       Term    Termination signal
       SIGUSR1   30,10,16    Term    User-defined signal 1
       SIGUSR2   31,12,17    Term    User-defined signal 2
       SIGCHLD   20,17,18    Ign     Child stopped or terminated
       SIGCONT   19,18,25    Cont    Continue if stopped
       SIGSTOP   17,19,23    Stop    Stop process
       SIGTSTP   18,20,24    Stop    Stop typed at terminal
       SIGTTIN   21,21,26    Stop    Terminal input for background process
       SIGTTOU   22,22,27    Stop    Terminal output for background process

>      The signals SIGKILL and SIGSTOP cannot be caught, blocked, or ignored.

ctrl + c 将会发出SIGINT信号。

