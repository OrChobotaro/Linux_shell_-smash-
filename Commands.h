#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string.h>
#include <list>

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
        int secondsElapsed;
        JobEntry(int jobId, pid_t pid, bool isStopped, char *commandLine, int secondsElapsed);
        JobEntry(const JobEntry& other);
        ~JobEntry() = default;
    };
    std::list<JobsList::JobEntry*> jobList;
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
    // TODO: Add extra methods or modify exisitng ones as needed
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
  ExternalCommand( char* cmd_line, bool isComplex, bool isBg, char **args, JobsList* jobs);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand( char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand( char* cmd_line);
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
// TODO: Add your data members public:
    char** plastPwd;
    std::string secondWord;
    int lengthArgs;
    bool* pathChanged;
  ChangeDirCommand(char* cmd_line, char** plastPwd, std::string secondWord, int lengthArgs, bool* pathChanged);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand( char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand( char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};


class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
public:
  QuitCommand( char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};



class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
    JobsList* jobs;
  JobsCommand(char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand( char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand( char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Bonus */
// TODO: Add your data members
 public:
  explicit TimeoutCommand( char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  ChmodCommand( char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  GetFileTypeCommand( char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
 public:
  SetcoreCommand( char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand( char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
    SmallShell();
public:
    std::string prompt;
    JobsList jobs;
    bool pathChanged;
    char *plastPwd;
    char **args;
    Command *CreateCommand( char* cmd_line);
    char *cmd_line;

    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

  ~SmallShell();
  void executeCommand( char* cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
