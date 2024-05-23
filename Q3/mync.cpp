#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void startTCPS(int port)
{
    
}


pair<string, int> getStringAndNumber(const string &input)
{
    pair<string, int> result;
    result.first = "";
    result.second = 0;
    if (input.find("TCPS") == 0)
    {
        //delete later
        result.first = "TCPS";
        result.second = stoi(input.substr(4));

        // start server
        int port = stoi(input.substr(4));
        startTCPS(port);
    }
    else if (input.find("TCPC") == 0)
    {

    }
    else
    {
        cerr << "invalid prameter !!";
        exit(1);
    }

    return result;
}

void executeProgram(string &program, string &args)
{
    vector<char *> execArgs;
    execArgs.push_back(&program[0]);
    execArgs.push_back(&args[0]);
    execArgs.push_back(nullptr);

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "Fork failed" << std::endl;
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        // Execute the program
        execvp(execArgs[0], execArgs.data());

        std::cerr << "Error executing " << program << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // Wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);
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

    for (int i = 4; i < argc; i++)
    {
        if (string(argv[i]).find("-i") == 0)
        {
            if (i + 1 < argc)
            {
               continue;
            }
        }
    }

    executeProgram(program, args);

    return 0;
}
