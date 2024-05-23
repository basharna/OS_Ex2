#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void executeProgram(string &program, string &args)
{
    vector<char *> execArgs;
    execArgs.push_back(&program[0]);
    execArgs.push_back(&args[0]);
    execArgs.push_back(nullptr);

    // pipes for communication
    // int inPipe[2], outPipe[2];
    // if (pipe(inPipe) < 0 || pipe(outPipe) < 0)
    // {
    //     std::cerr << "Pipe failed" << std::endl;
    //     exit(EXIT_FAILURE);
    // }

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "Fork failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // redirect stdin from parent process to child process
        // close(inPipe[1]);
        // close(outPipe[0]);
        // dup2(inPipe[0], STDIN_FILENO);
        // dup2(outPipe[1], STDOUT_FILENO);

        // Execute the program
        execvp(execArgs[0], execArgs.data());

        // close(inPipe[0]);
        // close(outPipe[1]);

        std::cerr << "Error executing " << program << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // redirect stdin from parent process to child process
        // close(inPipe[0]);
        // close(outPipe[1]);
        // dup2(inPipe[1], STDIN_FILENO);
        // dup2(outPipe[0], STDOUT_FILENO);
        // Wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);

        // close(inPipe[1]);
        // close(outPipe[0]);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 3 || std::string(argv[1]) != "-e")
    {
        std::cerr << "Usage: mync -e <program> [args...]" << std::endl;
        return 1;
    }

    string temp = argv[2];
    string program = "./" + temp;
    string args = argv[3];

    executeProgram(program, args);

    return 0;
}
