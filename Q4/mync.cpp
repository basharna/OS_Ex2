#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>
#include <signal.h>
#include <regex>

using namespace std;

pid_t childpid = -1;

void recv_TCPS(int sockfd)
{
    while (true)
    {
        char buf[10];
        int bytesReceived = recv(sockfd, buf, sizeof(buf), 0);
        if (bytesReceived <= 0)
        {
            close(sockfd); // Close client socket
            // kill child process
            if (kill(childpid, SIGKILL) == -1)
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
            close(sockfd); // Close client socket
            // kill child process
            if (kill(childpid, SIGKILL) == -1)
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

void handle_alarm(int s)
{
    if (childpid != -1)
    {
        kill(childpid, SIGKILL);
        exit(1);
    }
}

void setUpAlarm(int seconds)
{
    signal(SIGALRM, handle_alarm);
}

int startUDPS(int port, int timeOut)
{
    int sockfd;
    struct sockaddr_in address, addrCli;
    int addrlen = sizeof(address);
    socklen_t addrClen = sizeof(addrCli);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        cerr << "socket failed" << endl;
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

    return sockfd;
}

void recv_UDPS(int sockfd, int timeOut)
{
    char buffer[1024];
    setUpAlarm(timeOut);
    while (1)
    {
        int n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n < 0)
        {
            break;
        }
        alarm(timeOut);
    }
}

int startUDPC(string address, int port)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
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

    return sockfd;
}

void executeProgram(string &program, string &args, string type, int infd, int outfd, bool both)
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
        childpid = pid;
        if (infd != -1 && type == "TCPS")
        {
            recv_TCPS(infd);
        }
        else if (infd != -1 && type == "UDPS")
        {
        }
    }
}

bool validate_input(string &input)
{
    regex tcp_udp_regex("^(TCPS|UDPS)\\d+$");
    return regex_match(input, tcp_udp_regex);
}

bool validate_output(string &output)
{
    regex tcp_udp_regex("^(TCPC|UDPC)[^,]+,\\d+$");
    return regex_match(output, tcp_udp_regex);
}

void proccessArgs(int argc, char *argv[], int &opt, int &timeout, string &input, string &output, string &both, bool &input_flag, bool &output_flag, bool &both_flag)
{
    while ((opt = getopt(argc, argv, "i:o:b:t:e")) != -1)
    {
        switch (opt)
        {
        case 'i':
            input = optarg;
            input_flag = true;
            if (!validate_input(input))
            {
                cerr << "invalid input !!!";
                exit(1);
            }
            break;

        case 'o':
            output = optarg;
            output_flag = true;
            if (!validate_output(output))
            {
                cerr << "invalid output !!!";
                exit(1);
            }
            break;

        case 'b':
            both = optarg;
            both_flag = true;
            if (!validate_input(both))
            {
                cerr << "invalid input !!!";
                exit(1);
            }
            break;

        case 't':
            timeout = stoi(optarg);
            break;

        case 'e':
            break;

        default:
            cerr << "Usage: " << argv[0] << "[-i TCPS<port>|UDPS<port>] [-o TCPC<adress,port>|UDPC<adress,port>] [-b TCPS<port>|UDPS<port>] [-t <timeout>]";
            break;
        }
    }

    if ((input_flag || output_flag) && both_flag)
    {
        cerr << "invalid input !!!";
        exit(1);
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

    int opt;
    int timeout;
    string input;
    string output;
    string both;
    bool input_flag;
    bool output_flag;
    bool both_flag;
    proccessArgs(argc, argv, opt, timeout, input, output, both, input_flag, output_flag, both_flag);

    cout << input << " " << output << " " << both;
    return 0;
}
