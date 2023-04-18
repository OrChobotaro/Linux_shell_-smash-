#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    smash.getInstance().pidFg = -1; // initialize foreground pid
    char cmd_c[200];
    while(true) {
        std::cout << smash.prompt + "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        std::size_t length = cmd_line.copy(cmd_c, cmd_line.length(), 0);
        cmd_c[length] = '\0';
        smash.executeCommand(cmd_c);
    }
    return 0;
}