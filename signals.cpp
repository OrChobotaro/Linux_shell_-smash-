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

    bool jobExists = false;
    // checks if process exists in Jobs
    std::list<JobsList::JobEntry*>::iterator it;
    for (it = SmallShell::getInstance().jobs.jobList.begin(); it != SmallShell::getInstance().jobs.jobList.end(); ++it) {
        if((*it)->pid == pid){
            jobExists = true;
            (*it)->isStopped = true;
        }
    }
    if(!jobExists) {
        SmallShell::getInstance().jobs.addJob(cmd, true, pid);
    }

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
    cout << "smash: got an alarm" << endl;
   // iterates timeouts, deleting all commands with expired timeout
    std::list<TimeoutList::TimeoutEntry*>::iterator it;
    for (it = SmallShell::getInstance().timeouts.timeoutList.begin(); it != SmallShell::getInstance().timeouts.timeoutList.end();it++) {
        if((*it)->timestamp + (*it)->duration <= time(nullptr)) {

            pid_t pid = (*it)->pid;
            pid_t res = waitpid(pid, NULL, WNOHANG);
            if (res == 0) {
                cout << "smash: " << (*it)->cmd_line << " timed out!" << endl;
                if(kill(pid, 9) < 0){
                    perror("smash error: kill failed");
                }
            } else if (res < 0 && errno != ECHILD) {
                perror("smash error: waitpid");
            }

            it = SmallShell::getInstance().timeouts.timeoutList.erase(it);
            it--;
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

