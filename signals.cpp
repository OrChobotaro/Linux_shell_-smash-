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
  cout << "In handler harry potter" << endl;


   // iterates timeouts, deleting all commands with expired timeout
    std::list<TimeoutList::TimeoutEntry*>::iterator it;
    for (it = SmallShell::getInstance().timeouts.timeoutList.begin(); it != SmallShell::getInstance().timeouts.timeoutList.end();) {
        if((*it)->timestamp + (*it)->duration <= time(nullptr)){
            pid_t pid = (*it)->pid;
            cout << "deleting pid: " << pid << endl;
            if (kill(pid, 0) < 0) {
                if (errno != ESRCH) {
                    perror("smash error: kill failed");
                    return;
                }
                // do nothing, child doesn't exist
            } else {
                cout << "smash: " << (*it)->cmd_line << " timed out!" << endl;
                if(kill(pid, 9) < 0){
                    perror("smash error: kill failed");
                }
            }

            SmallShell::getInstance().timeouts.timeoutList.erase(it++);


//            if(res < 0){
//                perror("smash error: waitpid failed");
//            }
//            if(res == 0){
//                cout << "smash: " << (*it)->cmd_line << " timed out!" << endl;
//                if(kill(pid, 9) < 0){
//                    perror("smash error: kill failed");
//                }
//            }
        } else {
            ++it;
        }
    }

    // sends next alarm
    if (!SmallShell::getInstance().timeouts.timeoutList.empty()) {
        time_t nextAlarmTime = SmallShell::getInstance().timeouts.getNextAlarmTime();
        if(alarm(nextAlarmTime) < 0){
            perror("smash error: alarm failed");
        }
    }
}

