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

    struct sigaction sa;
    sa.sa_handler = alarmHandler;
    sa.sa_flags = SA_RESTART;

    if(sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("smash error: failed to set alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    smash.getInstance().pidFg = -1; // initialize foreground pid
    char cmd_c[COMMAND_ARGS_MAX_LENGTH];
    while(true) {
        std::cout << smash.prompt + "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        std::size_t length = cmd_line.copy(cmd_c, cmd_line.length(), 0);
        cmd_c[length] = '\0';

        if(cmd_line.find_first_not_of(" \t\n\v\f\r") != std::string::npos) {
            // There's a non-space in the command.
            smash.executeCommand(cmd_c);
        }
    }
    return 0;
}

/* PIAZZA LINKS:
// https://piazza.com/class/lfgtde16jfs1xm/post/174 - zombie children
 https://piazza.com/class/lfgtde16jfs1xm/post/175 - dup2 syscall?
 https://piazza.com/class/lfgtde16jfs1xm/post/153 - kill -19 should (stopped)
 https://piazza.com/class/lfgtde16jfs1xm/post/155 - error in external command
 https://piazza.com/class/lfgtde16jfs1xm/post/167 - chmod valid range

*/