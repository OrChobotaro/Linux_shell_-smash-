#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <list>
#include <thread>
#include <sys/stat.h>
#include <cmath>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


class JobsList {
public:
    class JobEntry {
    public:
        int jobId;
        pid_t pid;
        bool isStopped;
        char *commandLine;
        int startTime;
        JobEntry(int jobId, pid_t pid, bool isStopped, char *commandLine, int startTime);
        JobEntry(const JobEntry& other);
        ~JobEntry() = default;
    };
    std::list<JobsList::JobEntry*> jobList;
    int stoppedJobs; // updates in removeFinishedJobs
    JobsList() = default;
    ~JobsList() = default;
    void addJob( char* cmd, bool isStopped, pid_t pid);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
};

class Command {
 public:

     char* cmd_line;
    Command( char* cmd_line);

  virtual ~Command() = default; // TODO: our default
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand( char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
    bool isComplex;
    bool isBg;
    char **args;
    JobsList* jobs;
  ExternalCommand(char* cmd_line, bool isComplex, bool isBg, char **args, JobsList* jobs);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
public:
int isPipe;
size_t pipePosInString;
//char** argsLeft;
//char** argsRight;
char **args;
  PipeCommand(char* cmd_line, int isPipe, size_t pipePosInString , char** args);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 int isRedirection;
 size_t pos;
 public:
  RedirectionCommand(char* cmd_line, int isRedirection, size_t pos);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand {
public:
    class SmallShell;
    std::string* prompt;
    char* new_prompt;
    ChangePromptCommand( char* cmd_line, std::string* prompt, char* new_prompt);
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    char** plastPwd;
    char *secondWord;
    int lengthArgs;
    bool* pathChanged;
  ChangeDirCommand(char* cmd_line, char** plastPwd, char *secondWord, int lengthArgs, bool* pathChanged);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};


class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    JobsList* jobs;
    char* secondWord;
  QuitCommand(char* cmd_line, JobsList* jobs, char* secondWord);
  virtual ~QuitCommand() {}
  void execute() override;
};



class JobsCommand : public BuiltInCommand {
 public:
    JobsList* jobs;
  JobsCommand(char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs;
 char* secondWord;
 int argsLength;
 public:
  ForegroundCommand(char* cmd_line, JobsList* jobs, char* secondWord, int argsLength);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
    char* secondWord;
    int argsLength;
 public:
  BackgroundCommand(char* cmd_line, JobsList* jobs, char* secondWord, int argsLength);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutList {
public:
    class TimeoutEntry {
    public:
        char *cmd_line;
        pid_t pid;
        int duration;
        int timestamp;
        TimeoutEntry(char* cmd_line, pid_t pid, int duration, int timestamp);
        ~TimeoutEntry() = default;
    };
    std::list<TimeoutEntry*> timeoutList;
    TimeoutList() = default;
    ~TimeoutList() = default;
    void add(char *cmd_line, pid_t pid, int duration);
    void remove(pid_t pid);
    time_t getNextAlarmTime();
};

class TimeoutCommand : public BuiltInCommand {
    TimeoutList *timeouts;
    char *command;
    char *duration;
    int argsLength;
 public:
  explicit TimeoutCommand(char* cmd_line, char *duration, TimeoutList *timeouts, char *command, int argsLength);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
    char* newMod;
    char* pathToFile;
    int argsLength;
  ChmodCommand(char* cmd_line, char* newMod, char* pathToFile, int argsLength);
  virtual ~ChmodCommand() {}
  void execute() override;
  static int octalToDecimal(int octal);
};

class GetFileTypeCommand : public BuiltInCommand {
public:
    char *secondWord;
    int argsLength;
  GetFileTypeCommand(char* cmd_line, char *secondWord, int argsLength);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
 public:
    char* cmd_line;
    JobsList* jobs;
    char* jobid;
    char* core_num;
    int argsLength;
  SetcoreCommand(char* cmd_line, JobsList* jobs, char* jobid, char* core_num, int argsLength);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 public:
    JobsList* jobs;
    char* signum;
    char* jobid;
    int argsLength;
  KillCommand(char* cmd_line, JobsList* jobs, char* signum, char* jobid, int argsLength);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
    SmallShell();
public:
    std::string prompt;
    char *plastPwd;
    char **args;
    JobsList jobs;
    TimeoutList timeouts;
    bool pathChanged;
    Command *CreateCommand( char* cmd_line);
    char *cmd_line;
    pid_t pidFg;
    pid_t currentPid;
    bool isTimeout;
    int duration;
    char *timeoutCmdLine;
    bool isPipeExternal;
    bool inPipeCommand;

    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

  ~SmallShell() = default;
  void executeCommand( char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
