/**
	* Shell framework
	* course Operating Systems
	* Radboud University
	* v22.09.05

	Student names:
	- ...
	- ...
*/

/**
 * Hint: in most IDEs (Visual Studio Code, Qt Creator, neovim) you can:
 * - Control-click on a function name to go to the definition
 * - Ctrl-space to auto complete functions and variables
 */

// function/class definitions you are going to use
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <list>
#include <optional>

// although it is good habit, you don't have to type 'std' before many objects by including this line
using namespace std;

struct Command {
  vector<string> parts = {};
};

struct Expression {
  vector<Command> commands;
  string inputFromFile;
  string outputToFile;
  bool background = false;
};

// Parses a string to form a vector of arguments. The separator is a space char (' ').
vector<string> split_string(const string& str, char delimiter = ' ') {
  vector<string> retval;
  for (size_t pos = 0; pos < str.length(); ) {
    // look for the next space
    size_t found = str.find(delimiter, pos);
    // if no space was found, this is the last word
    if (found == string::npos) {
      retval.push_back(str.substr(pos));
      break;
    }
    // filter out consequetive spaces
    if (found != pos)
      retval.push_back(str.substr(pos, found-pos));
    pos = found+1;
  }
  return retval;
}

// wrapper around the C execvp so it can be called with C++ strings (easier to work with)
// always start with the command itself
// DO NOT CHANGE THIS FUNCTION UNDER ANY CIRCUMSTANCE
int execvp(const vector<string>& args) {
  // build argument list
  const char** c_args = new const char*[args.size()+1];
  for (size_t i = 0; i < args.size(); ++i) {
    c_args[i] = args[i].c_str();
  }
  c_args[args.size()] = nullptr;
  // replace current process with new process as specified
  int rc = ::execvp(c_args[0], const_cast<char**>(c_args));
  // if we got this far, there must be an error
  int error = errno;
  // in case of failure, clean up memory (this won't overwrite errno normally, but let's be sure)
  delete[] c_args;
  errno = error;
  return rc;
}

// Executes a command with arguments. In case of failure, returns error code.
int execute_command(const Command& cmd) {
  auto& parts = cmd.parts;
  if (parts.size() == 0)
    return EINVAL;

  // execute external commands
  int retval = execvp(parts);
  return retval ? errno : 0;
}

void display_prompt() {
  char buffer[512];
  char* dir = getcwd(buffer, sizeof(buffer));
  if (dir) {
    cout << "\e[32m" << dir << "\e[39m"; // the strings starting with '\e' are escape codes, that the terminal application interpets in this case as "set color to green"/"set color to default"
  }
  cout << "$ ";
  flush(cout);
}

string request_command_line(bool showPrompt) {
  if (showPrompt) {
    display_prompt();
  }
  string retval;
  getline(cin, retval);
  return retval;
}

// note: For such a simple shell, there is little need for a full-blown parser (as in an LL or LR capable parser).
// Here, the user input can be parsed using the following approach.
// First, divide the input into the distinct commands (as they can be chained, separated by `|`).
// Next, these commands are parsed separately. The first command is checked for the `<` operator, and the last command for the `>` operator.
Expression parse_command_line(string commandLine) {
  Expression expression;
  vector<string> commands = split_string(commandLine, '|');
  for (size_t i = 0; i < commands.size(); ++i) {
    string& line = commands[i];
    vector<string> args = split_string(line, ' ');
    if (i == commands.size() - 1 && args.size() > 1 && args[args.size()-1] == "&") {
      expression.background = true;
      args.resize(args.size()-1);
    }
    if (i == commands.size() - 1 && args.size() > 2 && args[args.size()-2] == ">") {
      expression.outputToFile = args[args.size()-1];
      args.resize(args.size()-2);
    }
    if (i == 0 && args.size() > 2 && args[args.size()-2] == "<") {
      expression.inputFromFile = args[args.size()-1];
      args.resize(args.size()-2);
    }
    expression.commands.push_back({args});
  }
  return expression;
}

int execute_composite_command (const Command& cmd) {
  // Check for empty expression
  auto& parts = cmd.parts;
  if (parts.size() == 0)
    return EINVAL;

  // extract all the single commands (handling the &'s)
  vector<Command> unit_commands;
  int prev = -1;
  for (int i = 0; i < (int) parts.size(); i++) {
    if  (parts[i] == "&") {
      vector<string> new_part(parts.begin() + prev + 1, parts.begin() + i);
      unit_commands.push_back( Command {new_part} );

      prev = i;
    }
  }

  // extract the last command
  vector<string> last_part(parts.begin() + prev + 1, parts.end());
  unit_commands.push_back( Command {last_part} );

  // executing the commands on different children
  vector<pid_t> children_cpids;
  pid_t cpid;

  for (int i = 0; i < (int) unit_commands.size(); i++) {
    cpid = fork();
    children_cpids.push_back(cpid);

    if (cpid == -1) {
      perror ("fork");
      exit(EXIT_FAILURE);
    }

    if (cpid == 0) {
      // child process

      int rc = execute_command(unit_commands[i]);

      if (rc != 0) {
        cerr << strerror(rc) << endl;
        abort();
      }

      exit(0);
    }

  }

  for (pid_t cpid : children_cpids) {
    waitpid(cpid, nullptr, 0);
  }

  return 0;
}

int execute_expression(Expression& expression) {
  // Check for empty expression
  if (expression.commands.size() == 0)
    return EINVAL;

  // Handle intern commands (like 'cd' and 'exit')
  if (expression.commands[0].parts[0] == "exit") 
    exit(0);

  if (expression.commands[0].parts[0] == "cd") {
    chdir(expression.commands[0].parts[1].c_str());
    if (expression.commands[0].parts.size() == 2)
      return 0;
    else {
      // remove 'cd "path" &' from the command
      expression.commands[0].parts.erase(expression.commands[0].parts.begin(), expression.commands[0].parts.begin() + 3);
    }

  }
    
  pid_t cpid;

  vector<int[2]> pipes(expression.commands.size() - 1);

  // Create all the pipes for inter-communication between child-processes
  for (int i = 0; i < (int) expression.commands.size() - 1; i++) {
    // "activate" pipe and check if functional
    if (pipe(pipes[i]) == -1) {
      perror("pipe");
      exit(EXIT_FAILURE);
    }
  }

  vector<pid_t> child_cpids;
  // execute all commands in pipe with a child-process per command
  for (int i = 0; i < (int) expression.commands.size(); i++) {
    //TODO: in parent process check if the command is cd or exit (or contains in case of & and handle)
    // behavior copied from ubuntu terminal. Only execute cd when at the end of pipe in an &

    // handle cd's in pipe
    if (i == (int) expression.commands.size() - 1 && expression.commands[i].parts[0] != "cd") { // at the end of piping and there is a command to interpret the pipe
      for (int j = 0; j < (int) expression.commands[i].parts.size(); j++) { // look for a cd
        if (expression.commands[i].parts[j] == "cd" && expression.commands[i].parts[j-1] == "&") {  // found a cd
            chdir(expression.commands[i].parts[j+1].c_str());

            // remove the cd command its argument and the & before
            expression.commands[i].parts.erase(expression.commands[i].parts.begin() + j - 1, expression.commands[i].parts.begin() + j + 2);
        }
      }
    }

    // if exit at the end of the command with a command before to accept the pipe then it is executed
    // handle exit in pipe
    if (i == (int) expression.commands.size() - 1 && expression.commands[i].parts[0] != "exit") { // at the end of piping and there is a command to interpret the pipe
      for (int j = 0; j < (int) expression.commands[i].parts.size(); j++) { // look for an exit
        if (expression.commands[i].parts[j] == "exit" && expression.commands[i].parts[j-1] == "&") {  // found an exit
            exit(0);
        }
      }
    }

    cpid = fork();
    child_cpids.push_back(cpid);

    if (cpid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }


    if (cpid == 0) {
      // child process

      if (i == 0 && expression.inputFromFile != "") { // First command in chain has a file input
        // Create a reading file descriptor
        int file_in = open(expression.inputFromFile.c_str(), O_RDONLY);

        // handle file error
        if (file_in < 0) {
          perror("Could not read from file");
          exit(EXIT_FAILURE);
        }

        // dup to std-in and handle dup2 error
        if (dup2(file_in, STDIN_FILENO) < 0) {
          perror ("dup2");
          exit(EXIT_FAILURE);
        }

        close (file_in);
      }

      if (i == (int) expression.commands.size() - 1 && expression.outputToFile != "") { // Last command in chain has output file
        // Create a writing file descriptor
        int file_out = open(expression.outputToFile.c_str(), O_WRONLY | O_TRUNC);

        // handle file error
        if (file_out < 0) {
          perror("Could not write to file");
          exit(EXIT_FAILURE);
        }

        // dup to std-out and handle dup2 error
        if (dup2(file_out, STDOUT_FILENO) < 0) {
          perror ("dup2");
          exit(EXIT_FAILURE);
        }

        close (file_out);
      }


      if (i > 0)  // Link the std-in to the read-end of the previous pipe
        if (dup2(pipes[i-1][0], STDIN_FILENO) == -1) {
          perror("dup2");
          exit(EXIT_FAILURE);
        }

      if (i < (int) expression.commands.size() - 1)  // Link the std-out with the write-en of the current pipe
        if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
          perror("dup2");
          exit(EXIT_FAILURE);
        }
      
      // close all the pipes in this process
      for (int j = 0; j < (int) pipes.size(); j++) {
        close(pipes[j][1]);
        close(pipes[j][0]);
      }

      // execute command
      int rc = execute_composite_command(expression.commands[i]);

      // handle errors
      if (rc != 0) {
        cerr << strerror(rc) << endl;
        abort(); 
      }

      exit(0);
    } 
  }

  // close all pipes in parent process (these are not used in this process)
  for (int i = 0; i < (int) pipes.size(); i++) {
    close(pipes[i][1]);
    close(pipes[i][0]);
  }


  // wait for all child processes to close
  for (pid_t pid : child_cpids) {
    waitpid(pid, nullptr, 0);
  }

  return 0;
}

// framework for executing "date | tail -c 5" using raw commands
// two processes are created, and connected to each other
int step1(bool showPrompt) {
  int pipefd[2];
  pipe(pipefd);
  pid_t child1 = fork();

  if (child1 == 0) {
    // redirecting the std-out to the write-end of the pipe
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]);
    close(pipefd[0]);

    // run the command
    Command cmd = {{string("date")}};
    int rc = execute_command(cmd);

    // handle errors
    if (rc != 0) {
      cerr << strerror(rc) << endl;
      abort(); 
    }

    exit(0);
  } 

  pid_t child2 = fork();
  if (child2 == 0) {
    // redirecting the std-in to the read-end of the pipe
    dup2(pipefd[0], STDIN_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    // executing the command
    Command cmd = {{string("tail"), string("-c"), string("5")}};
    int rc = execute_command(cmd);

    // handle errors
    if (rc != 0) {
      cerr << strerror(rc) << endl;
      abort(); // if the executable is not found, we should abort. (why?)
    }

    exit(0);
  }
  close(pipefd[1]);
  close(pipefd[0]);

  // free non used resources (why?)
  // wait on child processes to finish (why both?)
  waitpid(child1, nullptr, 0);
  waitpid(child2, nullptr, 0);
  return 0;
}

int shell(bool showPrompt) {
  //* <- remove one '/' in front of the other '/' to switch from the normal code to step1 code
  while (cin.good()) {
    string commandLine = request_command_line(showPrompt);
    Expression expression = parse_command_line(commandLine);
    int rc = execute_expression(expression);
    if (rc != 0)
      cerr << strerror(rc) << endl;
  }
  return 0;
  /*/
  return step1(showPrompt);
  //*/
}
