#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Commands.h"


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool _isCommandComplex(string cmd_s){
    size_t posQM = cmd_s.find("?");
    size_t posAS = cmd_s.find("*");

    if(posQM == string::npos && posAS == string::npos){
        return false;
    }
    return true;
}

int _isRedirection(string cmd_s, size_t *pos){
    size_t pos1 = cmd_s.find(">");
    size_t pos2 = cmd_s.find(">>");

    if(pos2 != string::npos){
        *pos = pos2;
        return 2;
    } else if(pos1 != string::npos){
        *pos = pos1;
        return 1;
    }
    *pos = string::npos;
    return 0;
}

SmallShell::SmallShell() : prompt("smash"), jobs(), pathChanged(false)  {
    plastPwd = new char[COMMAND_ARGS_MAX_LENGTH];
    args = new char*[COMMAND_MAX_ARGS];
    cmd_line = new char[COMMAND_ARGS_MAX_LENGTH];
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand( char* cmd_line) {
    // check if & exists
    bool inBg = false;
    char temp_cmd_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(temp_cmd_line, cmd_line);


    size_t pos;

    if (_isBackgroundCommand(temp_cmd_line)/* && isRedirection == 0*/) {
        inBg = true;
        _removeBackgroundSign(temp_cmd_line);
    }
    string cmd_s = _trim(string(temp_cmd_line));
    int isRedirection = _isRedirection(cmd_s, &pos);

    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    int lengthArgs = _parseCommandLine(cmd_s.c_str(), args);

    // checks if command is external complex
    bool isComplex = _isCommandComplex(cmd_s);

    if (isRedirection > 0) {
        return new RedirectionCommand(cmd_line, isRedirection, pos);
    }
  else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
      return new ChangePromptCommand(cmd_line, &prompt, args[1]);
  }
  else if (firstWord.compare("cd") == 0){
      return new ChangeDirCommand(cmd_line, &(this->plastPwd), args[1], lengthArgs, &pathChanged); // todo: change secondWord to char*
  }
  else if (firstWord.compare("fg") == 0){
      return new ForegroundCommand(cmd_line, &jobs, args[1], lengthArgs);
  }
  else if (firstWord.compare("bg") == 0){
      return new BackgroundCommand(cmd_line, &jobs, args[1], lengthArgs);
  }
  else if (firstWord.compare("jobs") == 0){
      return new JobsCommand(cmd_line, &jobs);
  }
  else if (firstWord.compare("quit") == 0){
      return new QuitCommand(cmd_line, &jobs, args[1]);
  }
  else if (firstWord.compare("kill") == 0){
      return new KillCommand(cmd_line, &jobs, args[1], args[2], lengthArgs);
  }
  else if (firstWord.compare("setcore") == 0){
      return new SetcoreCommand(cmd_line, &jobs, args[1], args[2], lengthArgs);
  }
  else if (firstWord.compare("getfileinfo") == 0){
      return new GetFileTypeCommand(cmd_line, args[1], lengthArgs);
  }
  else if (firstWord.compare("chmod") == 0){
      return new ChmodCommand(cmd_line, args[1], args[2], lengthArgs);
  }
  else {
    return new ExternalCommand(cmd_line, isComplex, inBg, args, &jobs);
  }
  return nullptr;
}

void SmallShell::executeCommand( char *cmd_line) {
   Command* cmd = CreateCommand(cmd_line);
   jobs.removeFinishedJobs();
    cmd->execute();
}

Command::Command( char *cmd_line): cmd_line(cmd_line) {}

BuiltInCommand::BuiltInCommand( char *cmd_line): Command(cmd_line) {}

//// pwd

GetCurrDirCommand::GetCurrDirCommand( char *cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char cwd[50];
    string str1 = getcwd(cwd, sizeof(cwd));
    string str2 = str1 + "\n";
    write(1, str2.c_str(), str2.length());

}

//// showpid

ShowPidCommand::ShowPidCommand(char *cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t pid = getpid();
    string pid_str = "smash pid is " + to_string(pid) + "\n";
//    write(1, pid_str.c_str(), pid_str.size());
    cout << "smash pid is " << to_string(pid) << endl;
}


//// chprompt

ChangePromptCommand::ChangePromptCommand(char *cmd_line, string* prompt, char* new_prompt): BuiltInCommand(cmd_line), prompt(prompt), new_prompt(new_prompt) {}

void ChangePromptCommand::execute() {
    if (new_prompt) {
        *prompt = new_prompt;
    } else {
        *prompt = "smash";
    }
}
//// cd
// TODO: handle syscall errors with perror


ChangeDirCommand::ChangeDirCommand(
        // TODO: handle case with no arguments
        char *cmd_line,
        char **plastPwd,
        char *secondWord,
        int lengthArgs,
        bool* pathChanged) : BuiltInCommand(cmd_line), plastPwd(plastPwd), secondWord(secondWord), lengthArgs(lengthArgs), pathChanged(pathChanged) {}


void ChangeDirCommand::execute() {
    char cwd[50];
    char* str = getcwd(cwd, sizeof(cwd));

    if(lengthArgs == 1){
        std::string cmd = cmd_line;
        std::string error = "smash error:> \""+ cmd +"\"\n";
        int errorLength = 16 + error.length();
        write(2, error.c_str() , errorLength);
        return;
    }
    std::string secondWordStr = secondWord;

    if(lengthArgs > 2)
        write(2, "smash error: cd: too many arguments\n", 36);
    else if(secondWordStr.compare("-") == 0){
        if(*pathChanged) {
            if(chdir(*plastPwd)==0) {
                strcpy(*plastPwd, str);
            }
        }
        else{
            write(2, "smash error: cd: OLDPWD not set\n", 32);
        }
    }
    else{
        if(chdir(secondWordStr.c_str())==0){
            strcpy(*plastPwd, str);
            *pathChanged = true;
        }
        else { //todo: invalid args is faliure of chdir or regular error?
            std::string cmd = cmd_line;
            std::string errorInvalidArgs = "smash error:> \"" + cmd + "\"\n";
            write(2, errorInvalidArgs.c_str(), 17 + cmd.length());
        }
    }
}


//// fg

ForegroundCommand::ForegroundCommand(char* cmd_line, JobsList* jobs, char* secondWord, int argsLength): BuiltInCommand(cmd_line),
                                                                                                            jobs(jobs), secondWord(secondWord), argsLength(argsLength){}

void ForegroundCommand::execute() { // todo: check if function works


    if(argsLength == 1){ // if there are no args
        if(!jobs->jobList.size()){
            write(2, "smash error: fg: jobs list is empty\n", 36);
            return;
        }
    }

    if(argsLength != 2){
        write(2, "smash error: fg: invalid arguments\n", 35);
    }

    std::string secondWordStr = secondWord;
    int intJobId;
    pid_t pid;

    try{
        intJobId = stoi(secondWordStr);
    } catch (...) {
        write(2, "smash error: bg: invalid arguments\n", 35);
        return;
    }

    std::string errorJobNotExists = "smash error: fg: job-id " + secondWordStr + " does not exist\n";

    if(intJobId <= 0){ // todo: check if it's the right error message
        write(2, errorJobNotExists.c_str(), 41);
        return;
    }

    std::list<JobsList::JobEntry*>::iterator it;
    for (it = jobs->jobList.begin(); it != jobs->jobList.end(); ++it) {
        if((*it)->jobId == intJobId){
            pid = (*it)->pid;

            // if stopped, sends SIGCONT
            if((*it)->isStopped){
                kill(pid, 18); // SIGCONT - 18
            }
            // waitpid - process moves to foreground
            SmallShell::getInstance().pidFg = pid;
            SmallShell::getInstance().cmd_line = (*it)->commandLine;
            jobs->jobList.erase(it); // erase the job
            if(waitpid(pid, nullptr, WUNTRACED) < 0){
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().cmd_line = nullptr;
            SmallShell::getInstance().pidFg = -1;
            return;
        }
    }

    write(2, errorJobNotExists.c_str(), 41);
}


//// bg

BackgroundCommand::BackgroundCommand(char *cmd_line, JobsList *jobs, char *secondWord, int argsLength) : BuiltInCommand(cmd_line),
                                                                                                         jobs(jobs), secondWord(secondWord), argsLength(argsLength){}

void BackgroundCommand::execute() {

    if(argsLength == 1){ // if there are no args
        std::list<JobsList::JobEntry*>::reverse_iterator it; // iterate and searching the stopped job with the greatest pid
        for (it = jobs->jobList.rbegin(); it != jobs->jobList.rend(); it++) {
            if((*it)->isStopped){
                pid_t pid = (*it)->pid;

                cout << (*it)->commandLine << " : " << pid << endl;

                kill(pid, 18); // SIGCONT - 18
                (*it)->isStopped = false; // continue running in the background

                return;
            }
        }
        write(2, "smash error: bg: there is no stopped jobs to resume\n", 52);
        return;
    }

    if(argsLength != 2){
        write(2, "smash error: bg: invalid arguments\n", 35);
    }

    std::string secondWordStr = secondWord;
    int intJobId;

    try{
        intJobId = stoi(secondWordStr);
    } catch (...) {
        write(2, "smash error: bg: invalid arguments\n", 35);
        return;
    }

    pid_t pid;

    std::string errorJobNotExists = "smash error: bg: job-id " + secondWordStr + " does not exist\n";

    if(intJobId <= 0){ // todo: which error message?
        write(2, errorJobNotExists.c_str(), 41);
        return;
    }

    // finding job id in jobs' list
    std::list<JobsList::JobEntry*>::iterator it;
    for (it = jobs->jobList.begin(); it != jobs->jobList.end(); ++it) {
        if((*it)->jobId == intJobId){
            pid = (*it)->pid;

            // if stopped, sends SIGCONT
            if((*it)->isStopped){
                cout << (*it)->commandLine << " : " << pid << endl;
                kill(pid, 18); // SIGCONT - 18
                (*it)->isStopped = false; // continue running in the background
            } else {
                std::string errorAlreadyBg = "smash error: bg: job-id " + secondWordStr + " is already running in the background\n";
                write(2, errorAlreadyBg.c_str(), 63);
                return;
            }

            return;
        }
    }

    write(2, errorJobNotExists.c_str(), 41);
}

//// external command

ExternalCommand::ExternalCommand( char *cmd_line, bool isComplex, bool isBg, char **args, JobsList* jobs) :
                                    Command(cmd_line), isComplex(isComplex), isBg(isBg) , args(args), jobs(jobs){}


void ExternalCommand::execute() {
    if(!isComplex && !isBg) {
        pid_t pid = fork();
        if(pid < 0){
            perror("smash error: fork failed");
        }
        if(pid == 0){
            setpgrp();
            string cmd = args[0];
            execvp(cmd.c_str(), args);
            perror("smash error: execvp failed");
            exit(0);
        } else if(pid > 0){
            SmallShell::getInstance().pidFg = pid; // save fg process pid
            SmallShell::getInstance().cmd_line = cmd_line; // save fg cmd line
            if(waitpid(pid, nullptr, WUNTRACED) < 0){
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().pidFg = -1;
        }

    } else if(!isComplex && isBg) {
        pid_t pid = fork();
        if(pid < 0){
            perror("smash error: fork failed");
        }
        if(pid == 0) {
            setpgrp();
            string cmd = args[0];
            execvp(cmd.c_str(), args);
            perror("smash error: execvp failed");
            exit(1); // todo: exit(0)?
        } else if (pid > 0) {
            jobs->addJob(cmd_line, false, pid);
        }

    } else if (isComplex && !isBg) {
        pid_t pid = fork();
        if(pid < 0){
            perror("smash error: fork failed");
        }
        if (pid == 0) {
            setpgrp();
            char* complexArgs[4];
            char temp_cmd_line[COMMAND_ARGS_MAX_LENGTH];
            strcpy(temp_cmd_line, cmd_line);
            complexArgs[0] = (char*)"/bin/bash";
            complexArgs[1] = (char*)"-c";
            complexArgs[2] = temp_cmd_line;
            complexArgs[3] = NULL;
            execv(complexArgs[0], complexArgs);
            perror("smash error: execvp failed");
            exit(1);

        } else if (pid > 0) {
            SmallShell::getInstance().pidFg = pid; // save fg process pid
            SmallShell::getInstance().cmd_line = cmd_line; // save fg cmd line
            if(waitpid(pid, nullptr, WUNTRACED) < 0){
                perror("smash error: waitpid failed");
            }
            SmallShell::getInstance().pidFg = -1;
        }

    } else if(isComplex && isBg) {
        pid_t pid = fork();
        if(pid < 0){
            perror("smash error: fork failed");
        }
        if (pid == 0) {
            setpgrp();
            char* complexArgs[4];
            char temp_cmd_line[COMMAND_ARGS_MAX_LENGTH];
            strcpy(temp_cmd_line, cmd_line);
            complexArgs[0] = (char*)"/bin/bash";
            complexArgs[1] = (char*)"-c";
            complexArgs[2] = temp_cmd_line;
            complexArgs[3] = NULL;
            execv(complexArgs[0], complexArgs);
            perror("smash error: execvp failed");
            exit(1);

        } else if (pid > 0) {
            jobs->addJob(cmd_line, false, pid);
        }
    }
}


//// Job Entry

JobsList::JobEntry::JobEntry(int jobId, int pid, bool isStopped,  char *commandLine, int startTime):
        jobId(jobId), pid(pid), isStopped(isStopped), startTime(startTime) {
    this->commandLine = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(this->commandLine, commandLine);
}

JobsList::JobEntry::JobEntry(const JobEntry &other) {
    jobId = other.jobId;
    pid = other.pid;
    isStopped = other.isStopped;
    startTime = other.startTime;
    strcpy(commandLine, other.commandLine);
}
//// jobs

JobsCommand::JobsCommand(char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void JobsCommand::execute() {
    jobs->printJobsList();
}

void JobsList::addJob(char* cmd, bool isStopped, pid_t pid){
    int jobId;
    if(!jobList.empty()){
        jobId = jobList.back()->jobId + 1;
    } else {
        jobId = 1;
    }

    time_t startTime = time(nullptr);
    JobEntry* newJob = new JobEntry(jobId, pid, isStopped, cmd, startTime);
    jobList.push_back(newJob);
}

void JobsList::printJobsList() {
    time_t currentTime = time(nullptr);

    std::list<JobEntry*>::iterator it;
    for (it = jobList.begin(); it != jobList.end(); ++it){
        time_t startTime = (*it)->startTime;
        double secondsElapsed = difftime(currentTime, startTime);
        string secondsElapsed_s = to_string(static_cast<int>(secondsElapsed + 0.5));
        string jobId = to_string((*it)->jobId);
        string pid = to_string((*it)->pid);
        bool isStopped = (*it)->isStopped;
        string entry = "[" + jobId + "] " + (*it)->commandLine + " : " + pid + " " + secondsElapsed_s + " secs";
        if(isStopped){
            entry = entry + " (stopped)";
        }
        std::cout << entry << endl;
    }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    std::list<JobsList::JobEntry*>::iterator it;
    for (it = jobList.begin(); it != jobList.end(); ++it){
        if ((*it)->jobId == jobId) {
            return *it;
        }
    }

    return nullptr;
}

//// quit

QuitCommand::QuitCommand(char *cmd_line, JobsList *jobs, char* secondWord): BuiltInCommand(cmd_line), jobs(jobs), secondWord(secondWord) {}

void QuitCommand::execute() {
    if (secondWord && strcmp(secondWord, "-kill") == 0) {
        jobs->killAllJobs();
    }

    // kill wasn't entered
    exit(0);
}

void JobsList::killAllJobs() {
    std::list<JobEntry*>::iterator it;
    cout << "sending SIGKILL signal to " << jobList.size() << " jobs:" << endl;
    for (it = jobList.begin(); it != jobList.end(); ++it){
        pid_t pid = (*it)->pid;
        char* cmd_line = (*it)->commandLine;
        cout << pid << ": " << cmd_line << endl;
        kill(pid, 9);
    }
}

//// kill

KillCommand::KillCommand(char *cmd_line, JobsList *jobs, char *signum, char *jobid, int argsLength):
    BuiltInCommand(cmd_line), jobs(jobs), signum(signum), jobid(jobid), argsLength(argsLength) {}

void KillCommand::execute() {
    if (argsLength != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int signumInt, jobidInt;
    try {
        signumInt = stoi(signum);
        jobidInt = stoi(jobid);
    } catch (...) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    int signumPositive = -1 * signumInt;
    JobsList::JobEntry* jobToKill = jobs->getJobById(jobidInt);
    if (signumPositive < 1 || signumPositive > 31 || jobidInt < 0) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    if (!jobToKill) {
        cerr << "smash error: kill: job-id " << jobidInt << " does not exist" << endl;
        return;
    }

    kill(jobToKill->pid, signumPositive);
}

void JobsList::removeFinishedJobs() { // todo: check stopped jobs variable.
    this->stoppedJobs = 0;

    std::list<JobEntry*>::iterator it;
    for (it = jobList.begin(); it != jobList.end(); ++it){
        pid_t pid = (*it)->pid;
        pid_t res = waitpid(pid, NULL, WNOHANG);
        if(res < 0){
            perror("smash error: waitpid failed");
        }

        if((*it)->isStopped){
            this->stoppedJobs++;
        }

        if(res > 0){ // if res greater than 0, the job finished and can be deleted.
            it = jobList.erase((it));
            it--;
        }
    }
    return;

}

//// setcore
SetcoreCommand::SetcoreCommand(char *cmd_line, JobsList *jobs, char *jobid, char *core_num, int argsLength):
        BuiltInCommand(cmd_line), jobs(jobs), jobid(jobid), core_num(core_num), argsLength(argsLength) {}

void SetcoreCommand::execute() {
    if (argsLength != 3) {
        cerr << "smash error: setcore: invalid arguments" << endl;
        return;
    }
    int jobidInt, coreNumInt;
    try {
        jobidInt = stoi(jobid);
        coreNumInt = stoi(core_num);
    } catch (...) {
        cerr << "smash error: setcore: invalid arguments" << endl;
        return;
    }

    const auto core_count = std::thread::hardware_concurrency();

    if (coreNumInt < 0 || coreNumInt > core_count - 1) {
        cerr << "smash error: setcore: invalid core number" << endl;
    }

    JobsList::JobEntry* jobToSet = jobs->getJobById(jobidInt);
    if (!jobToSet) {
        cerr << "smash error: setcore: job-id " << jobidInt << " does not exist" << endl;
        return;
    }

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(coreNumInt, &mask);
    sched_setaffinity(jobToSet->pid, sizeof(mask), &mask);
}


//// getfiletype


GetFileTypeCommand::GetFileTypeCommand(char *cmd_line, char *secondWord, int argsLength)
            : BuiltInCommand(cmd_line), secondWord(secondWord), argsLength(argsLength){}


void GetFileTypeCommand::execute() {
    if(argsLength != 2){
        write(1, "smash error: gettype: invalid aruments", 38);
        return;
    }


    struct stat type;

    int res = stat(secondWord, &type);

    std::string fileType;

    if(S_ISREG(type.st_mode)){
        fileType = "regular file";
    } else if(S_ISDIR(type.st_mode)){
        fileType = "directory";
    } else if(S_ISBLK(type.st_mode)){
        fileType = "block device";
    } else if(S_ISCHR(type.st_mode)){
        fileType = "character device";
    } else if(S_ISFIFO(type.st_mode)){
        fileType = "FIFO";
    } else if(S_ISLNK(type.st_mode)){
        fileType = "symbolic link";
    } else if(S_ISSOCK(type.st_mode)){
        fileType = "socket";
    } else {
        write(1, "smash error: gettype: invalid aruments", 38);
        return;
    }

    cout << secondWord << "'s type is \"" << fileType << "\" and takes up " << type.st_size << " bytes" << endl;

}

//// chmod
ChmodCommand::ChmodCommand(char *cmd_line, char *newMod, char *pathToFile, int argsLength):
BuiltInCommand(cmd_line), newMod(newMod), pathToFile(pathToFile), argsLength(argsLength) {}

void ChmodCommand::execute() {
    if (argsLength != 3) {
        cerr << "smash error: chmod: invalid arguments" << endl;
    }
    int newModInt, decimalNewMod;
    try {
        newModInt = stoi(newMod);
    } catch (...) {
        cerr << "smash error: chmod: invalid arguments" << endl;
        return;
    }

    decimalNewMod = ChmodCommand::octalToDecimal(newModInt);
    if (chmod(pathToFile, decimalNewMod) < 0) {
        perror("smash error: chmod failed");
    }
}

int ChmodCommand::octalToDecimal(int octalNumber) {
    int decimalNumber = 0, i = 0;
    while (octalNumber != 0) {
        decimalNumber += (octalNumber % 10) * pow(8, i);
        ++i;
        octalNumber /= 10;
    }
    return decimalNumber;

}

RedirectionCommand::RedirectionCommand(char *cmd_line, int isRedirection, size_t pos)
                            : Command(cmd_line), isRedirection(isRedirection), pos(pos){}

void RedirectionCommand::execute() {

    // if command does not fork
    // override stdout, run command, restore stdout

    char command[COMMAND_ARGS_MAX_LENGTH];
    strncpy(command, cmd_line, pos);
    command[pos] = '\0';

    char file[200];
    strcpy(file, cmd_line + pos + isRedirection);
    string fileStr = file;

    fileStr = _trim(fileStr);
    fileStr.copy(file, fileStr.length());
    file[fileStr.length()] = '\0';

    int stdoutSaved = dup(1);
    close(1); // close stdout

    if (isRedirection == 1) {
        open(file, O_WRONLY | O_CREAT);

    } else if(isRedirection == 2){
        open(file, O_WRONLY | O_CREAT | O_APPEND);
    }

    SmallShell::getInstance().executeCommand(command);

    dup2(stdoutSaved, 1);
    close(stdoutSaved);

    // if command forks
        // run external command, with special parameter

}

