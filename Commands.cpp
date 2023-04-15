#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : prompt("smash"), jobs(), pathChanged(false)  {
    plastPwd = new char[COMMAND_ARGS_MAX_LENGTH];
    args = new char*[COMMAND_MAX_ARGS];
    cmd_line = new char[COMMAND_ARGS_MAX_LENGTH];
}

SmallShell::~SmallShell() {
    delete[] plastPwd;
    delete[] args;
    delete[] cmd_line;
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand( char* cmd_line) {
    // check if & exists
    bool inBg = false;
    char temp_cmd_line[COMMAND_ARGS_MAX_LENGTH];
    strcpy(temp_cmd_line, cmd_line);
    if (_isBackgroundCommand(temp_cmd_line)) {
        inBg = true;
        _removeBackgroundSign(temp_cmd_line);
    }
    string cmd_s = _trim(string(temp_cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  int lengthArgs = _parseCommandLine(cmd_s.c_str(), args);

  // checks if command is external complex
    bool isComplex = _isCommandComplex(cmd_s);

  if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
      return new ChangePromptCommand(cmd_line, &prompt, args[1]);
  }
  else if (firstWord.compare("cd") == 0){
      return new ChangeDirCommand(cmd_line, &(this->plastPwd), args[1], lengthArgs, &pathChanged);
  }
  else if (firstWord.compare("jobs") == 0){
      return new JobsCommand(cmd_line, &jobs);
  }
//  else if ...
//  .....
  else {

    return new ExternalCommand(cmd_line, isComplex, inBg, args, &jobs);

  }

  return nullptr;
}

void SmallShell::executeCommand( char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
   Command* cmd = CreateCommand(cmd_line);
   cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
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

ShowPidCommand::ShowPidCommand( char *cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t pid = getpid();
    string pid_str = "smash pid is " + to_string(pid) + "\n";
    write(1, pid_str.c_str(), pid_str.size());
}


//// chprompt

ChangePromptCommand::ChangePromptCommand( char *cmd_line, string* prompt, char* new_prompt): BuiltInCommand(cmd_line), prompt(prompt), new_prompt(new_prompt) {}

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
        std::string secondWord,
        int lengthArgs,
        bool* pathChanged) : BuiltInCommand(cmd_line), plastPwd(plastPwd), secondWord(secondWord), lengthArgs(lengthArgs), pathChanged(pathChanged) {}


void ChangeDirCommand::execute() {
    char cwd[50];
    char* str = getcwd(cwd, sizeof(cwd));

    if(lengthArgs > 2)
        write(2, "smash error: cd: too many arguments\n", 36);
    else if(secondWord.compare("-") == 0){
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
        if(chdir(secondWord.c_str())==0){
            strcpy(*plastPwd, str);
            *pathChanged = true;
        }
        else //todo: remove faliure
            cout << "faliure :(";
    }
}

//// external command

ExternalCommand::ExternalCommand( char *cmd_line, bool isComplex, bool isBg, char **args, JobsList* jobs) :
                                    Command(cmd_line), isComplex(isComplex), isBg(isBg) , args(args), jobs(jobs){}


void ExternalCommand::execute() {

    if(!isComplex && !isBg) {
        pid_t pid = fork();
        if(pid == 0){
            setpgrp();
            string bin = "/bin/";
            string cmd(args[0]);
            string concat = bin + cmd;
            execv(concat.c_str(), args);
            cout << "simple EXECV FAILED" << endl;
            exit(1);
        } else if(pid > 0){
            waitpid(pid, nullptr, 0);
        }

    } else if(!isComplex && isBg) {
        pid_t pid = fork();
        if(pid == 0) {
            setpgrp();
            string bin = "/bin/";
            string cmd(args[0]);
            string concat = bin + cmd;
            execv(concat.c_str(), args);
            cout << "simple EXECV FAILED" << endl;
            exit(0);
        } else if (pid > 0) {
            jobs->addJob(cmd_line, false, pid);
        }

        // same without wait
        return;

    } else if (isComplex && !isBg) {
        pid_t pid = fork();
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
            cout << "complex EXECV FAILED" << endl;
            exit(1);

        } else if (pid > 0) {
            waitpid(pid, nullptr, 0);
        }

    } else if(isComplex && isBg) {
        // same without wait
        return;
    }
}


//// Job Entry

JobsList::JobEntry::JobEntry(int jobId, int pid, bool isStopped,  char *commandLine, int secondsElapsed):
        jobId(jobId), pid(pid), isStopped(isStopped), secondsElapsed(secondsElapsed) {
    this->commandLine = new char[COMMAND_ARGS_MAX_LENGTH];
    strcpy(this->commandLine, commandLine);
}

JobsList::JobEntry::JobEntry(const JobEntry &other) {
    jobId = other.jobId;
    pid = other.pid;
    isStopped = other.isStopped;
    secondsElapsed = other.secondsElapsed;
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
    std::list<JobEntry*>::iterator it;
    for (it = jobList.begin(); it != jobList.end(); ++it){
        std::cout << (*it)->pid << endl;
        std::cout << (*it)->jobId << endl;
        std::cout << (*it)->commandLine << endl;
    }
}