#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>


using namespace std;

int startTCPS(int port)
{
    int sockfd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        cerr << "socket failed" << endl;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        cerr << "setsockopt failed" << endl;
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cerr << "bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 3) < 0)
    {
        cerr << "listen failed" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Listening on port " << port << endl;

    if ((client_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        cerr << "accept failed" << endl;
        exit(EXIT_FAILURE);
    }

    return client_socket;
}

int startTCPC(string address, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cerr << "socket failed" << endl;
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (address == "localhost")
    {
        address = "127.0.0.1";
    }

    if (inet_pton(AF_INET, address.c_str(), &serv_addr.sin_addr) <= 0)
    {
        cerr << "inet_pton failed" << endl;
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "connect failed" << endl;
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

void proccessArgs(const string &flag, const string &input, int &infd, int &outfd)
{
    int port;
    string address;
    if (input.find("TCPS") == 0)
    {
        port = stoi(input.substr(4));
    }
    else if (input.find("TCPC") == 0)
    {
        size_t pos;
        if ((pos = input.find(",")) == string::npos)
        {
            cerr << "invalid prameter !!" << endl;
            exit(1);
        }
        address = input.substr(4, pos - 4);
        string temp = input.substr(pos + 1);
        if (temp.empty())
        {
            cerr << "invalid prameter !!" << endl;
            exit(1);
        }
        port = stoi(temp);
    }
    else
    {
        cerr << "invalid prameter !!" << endl;
        exit(1);
    }

    if (flag == "-i" && address.empty())
    {
        // start server
        infd = startTCPS(port);
    }
    else if (flag == "-o" && !address.empty())
    {
        // start client
        outfd = startTCPC(address, port);
    }
    else if (flag == "-b" && address.empty())
    {
        infd = startTCPS(port);
    }
    else
    {
        cerr << "invalid prameter !!" << endl;
        exit(1);
    }
}

void executeProgram(string &program, string &args, int infd, int outfd, bool both)
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
        // redirect stdin of child process to infd
        if (infd != -1)
        {
            dup2(infd, STDIN_FILENO);
            if (both)
            {
                dup2(infd, STDOUT_FILENO);
            }
        }
        close(infd);
        // redirect stdout of child process to outfd
        if (outfd != -1)
        {
            dup2(outfd, STDOUT_FILENO);
        }
        close(outfd);

        // Execute the program
        execvp(execArgs[0], execArgs.data());

        std::cerr << "Error executing " << program << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        // check if client disconnected
        if (infd != -1)
        {
            while (true)
            {
                cout << "im here" << endl;
                char buf[10];
                int bytesReceived = recv(infd, buf, sizeof(buf), 0);
                if (bytesReceived <= 0)
                {
                    close(infd); // Close client socket
                    // kill child process
                    if (kill(pid, SIGKILL) == -1)
                    {
                        // Error handling
                        std::cerr << "Failed to kill child process" << std::endl;
                        exit(1);
                    }
                    cout << "Client disconnected" << endl;
                    exit(1);
                }
                else if (string(buf) == "exit\n")
                {
                    close(infd); // Close client socket
                    // kill child process
                    if (kill(pid, SIGKILL) == -1)
                    {
                        // Error handling
                        std::cerr << "Failed to kill child process" << std::endl;
                        exit(1);
                    }
                    cout << "Client disconnected" << endl;
                    exit(1);
                }
            }
        }

        // wait
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
    set<string> flags;
    int infd = -1;
    int outfd = -1;

    // ./mync -e ./program <arg> -i TCPS1234 -o TCPC127.0.0.1,1234
    for (int i = 4; i < argc; i++)
    {
        if (string(argv[i]).find("-i") == 0 || string(argv[i]).find("-o") == 0 || string(argv[i]).find("-b") == 0)
        {
            if (i + 1 < argc)
            {
                flags.insert(argv[i]);
                if (flags.size() > 2 || (flags.size() == 2 && flags.find("-d") != flags.end()))
                {
                    cerr << "invalid prameter !!";
                    exit(1);
                }
                proccessArgs(argv[i], argv[i + 1], infd, outfd);
                i++;
            }
            else
            {
                cerr << "invalid prameter !!";
                exit(1);
            }
        }
    }

    bool both = false;
    if (flags.find("-b") != flags.end())
    {
        both = true;
    }
    executeProgram(program, args, infd, outfd, both);

    return 0;
}
