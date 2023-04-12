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

bool _isBackgroundComamnd(const char* cmd_line) {
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

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : prompt("smash") {
  plastPwd = new char[200];
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  char *args[COMMAND_MAX_ARGS];
  int lengthArgs = _parseCommandLine(cmd_s.c_str(), args);

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
      return new ChangeDirCommand(cmd_line, &(this->plastPwd), args[1], lengthArgs);
  }
//  else if ...
//  .....
//  else {
//    return new ExternalCommand(cmd_line);
//  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
   Command* cmd = CreateCommand(cmd_line);
   cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}

Command::Command(const char *cmd_line): cmd_line(cmd_line) {}

BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line) {}

//// pwd

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char cwd[50];
    string str1 = getcwd(cwd, sizeof(cwd));
    string str2 = str1 + "\n";
    write(1, str2.c_str(), str2.length());
}

//// showpid

ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    pid_t pid = getpid();
    string pid_str = "smash pid is " + to_string(pid) + "\n";
    write(1, pid_str.c_str(), pid_str.size());
}

ChangePromptCommand::ChangePromptCommand(const char *cmd_line, string* prompt, char* new_prompt): BuiltInCommand(cmd_line), prompt(prompt), new_prompt(new_prompt) {}

void ChangePromptCommand::execute() {
    if (new_prompt) {
        *prompt = new_prompt;
    } else {
        *prompt = "smash";

//// cd

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd, std::string secondWord, int lengthArgs) : BuiltInCommand(cmd_line), plastPwd(plastPwd)
                                                                                                , secondWord(secondWord), lengthArgs(lengthArgs){}

void ChangeDirCommand::execute() {
    char cwd[50];
    char* str = getcwd(cwd, sizeof(cwd));

    if(lengthArgs > 2)
        write(1, "smash error: cd: too many arguments", 35);
    else if(secondWord.compare("-") == 0){
        if(!(*plastPwd))
            write(1, "smash error: cd: OLDPWD not set", 31);
        else{
            cout << "chdir: " << chdir(*plastPwd);
            strcpy(*plastPwd, str);
        }
    }
    else{
        if(chdir(secondWord.c_str())==0){
            strcpy(*plastPwd, str);
        }
        else //todo: remove faliure
            cout << "faliure :(";
    }
}