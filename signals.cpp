#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
    write(1, "smash: got ctrl-Z\n", 18);

    pid_t pid = SmallShell::getInstance().pidFg;

    if(pid < 0){ // if there is no process in foreground
        return;
    }
    kill(pid, 19); // SIGSTOP - 19

    // add stopped job to jobs' list
    char *cmd = SmallShell::getInstance().cmd_line;
    SmallShell::getInstance().jobs.addJob(cmd, true, pid);
    std::string pidStr = std::to_string(pid);
    std::string message = "smash: process " + pidStr + " was stopped\n";
    write(1, message.c_str(), message.length());
    return;
}

void ctrlCHandler(int sig_num) {
    write(1, "smash: got ctrl-C\n", 18);

    pid_t pid = SmallShell::getInstance().pidFg;
    if(pid < 0){ // if there is no process in foreground
        return;
    }
    kill(pid, 9); // SIGKILL - 9

    std::string pidStr = std::to_string(pid);
    std::string message = "smash: process " + pidStr + " was killed\n";
    write(1, message.c_str(), message.length());
    return;
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

