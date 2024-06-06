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
#include <fcntl.h>

#define max_clients 10
using namespace std;

pid_t childpid = -1;

struct OutputInfo
{
    string type;
    string address;
    int port;
};

struct InputInfo
{
    string type;
    int port;
};

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

void handleFork(string &program, string &args, int infd, int outfd, bool both)
{
    vector<char *> execArgs;
    execArgs.push_back(&program[0]);
    execArgs.push_back(&args[0]);
    execArgs.push_back(nullptr);

    if (infd != -1)
    {
        dup2(infd, STDIN_FILENO);
        if (both)
        {
            dup2(infd, STDOUT_FILENO);
        }
    }
    // redirect stdout of child process to outfd
    if (outfd != -1)
    {
        dup2(outfd, STDOUT_FILENO);
    }

    // Execute the program
    execvp(execArgs[0], execArgs.data());

    close(infd);
    close(outfd);

    std::cerr << "Error executing " << program << std::endl;
    exit(EXIT_FAILURE);
}

int startTCPMUXS(int port, string &program, string &args, int outputSock, bool both)
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    fd_set read_fds, master_fds;
    socklen_t addr_len;

    // Create a TCP socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow reuse of address and port
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, max_clients) < 0)
    {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Initialize the master file descriptor set
    FD_ZERO(&master_fds);
    FD_SET(server_socket, &master_fds);

    std::cout << "Server listening on port " << port << std::endl;

    while (true)
    {
        read_fds = master_fds;

        // Wait for an activity on one of the sockets
        if (select(FD_SETSIZE, &read_fds, nullptr, nullptr, nullptr) < 0)
        {
            perror("select");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        // Check if there is a new connection request
        for (int i = 0; i < FD_SETSIZE; ++i)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == server_socket)
                {
                    // Accept a new connection
                    addr_len = sizeof(client_addr);
                    if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len)) < 0)
                    {
                        perror("accept");
                        continue;
                    }

                    cout << "New connection from "
                         << inet_ntoa(client_addr.sin_addr) << ":"
                         << ntohs(client_addr.sin_port) << std::endl;

                    // Fork a new process to handle the client
                    pid_t pid = fork();
                    if (pid < 0)
                    {
                        perror("fork");
                        close(client_socket);
                    }
                    else if (pid == 0)
                    {
                        // Child process
                        close(server_socket);
                        if (outputSock == -2)
                        {
                            outputSock = client_socket;
                        }
                        handleFork(program, args, client_socket, outputSock, both);
                        exit(0);
                    }
                    else
                    {
                        close(client_socket);
                        while (waitpid(-1, nullptr, WNOHANG) > 0)
                            ;
                    }
                }
            }
        }
    }

    // Close the server socket
    close(server_socket);
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

int startUDPS(int port, int timeOut)
{
    int sockfd;
    struct sockaddr_in address;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
    {
        cerr << "socket failed" << endl;
        exit(EXIT_FAILURE);
    }

    // set socket to reuse address and port
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0)
    {
        cerr << "setsockopt failed" << endl;
        exit(EXIT_FAILURE);
    }

    // set timeout
    if (timeOut != -1)
    {
        struct timeval tv;
        tv.tv_sec = timeOut;
        tv.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv) < 0)
        {
            cerr << "setsockopt failed" << endl;
            exit(EXIT_FAILURE);
        }
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

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "connect failed" << endl;
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
        // redirect stdout of child process to outfd
        if (outfd != -1)
        {
            dup2(outfd, STDOUT_FILENO);
        }

        // Execute the program
        execvp(execArgs[0], execArgs.data());

        close(infd);
        close(outfd);

        std::cerr << "Error executing " << program << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        childpid = pid;
        int status;
        waitpid(pid, &status, 0);
        exit(0);
    }
}

bool validate_input(string &input)
{
    regex tcp_udp_regex("^(TCPS|TCPMUXS|UDPS)\\d+$");
    return regex_match(input, tcp_udp_regex);
}

bool validate_output(string &output)
{
    regex tcp_udp_regex("^(TCPC|UDPC)[^,]+,\\d+$");
    return regex_match(output, tcp_udp_regex);
}

OutputInfo extract_output_info(const std::string &output)
{
    OutputInfo info;

    regex pattern("^(TCPC|UDPC)([^,]+),([0-9]+)$");

    smatch match;
    if (regex_match(output, match, pattern))
    {
        info.type = match[1].str();

        info.address = match[2].str();

        info.port = stoi(match[3].str());
    }

    return info;
}

InputInfo extract_input_info(const std::string &input)
{
    InputInfo info;

    regex pattern("^(TCPS|TCPMUXS|UDPS)([0-9]+)$");

    smatch match;
    if (regex_match(input, match, pattern))
    {
        info.type = match[1].str();
        info.port = stoi(match[2].str());
    }

    return info;
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
                cerr << "invalid -i argument" << endl;
                exit(1);
            }
            break;

        case 'o':
            output = optarg;
            output_flag = true;
            if (!validate_output(output))
            {
                cerr << "invalid -o argument" << endl;
                exit(1);
            }
            break;

        case 'b':
            both = optarg;
            both_flag = true;
            if (!validate_input(both))
            {
                cerr << "invalid -b argument" << endl;
                exit(1);
            }
            break;

        case 't':
            // check if optrarg is a number and not letters
            for (size_t i = 0; i < strlen(optarg); i++)
            {
                if (!isdigit(optarg[i]))
                {
                    cerr << "timeout must be a positive integer" << endl;
                    exit(1);
                }
            }

            timeout = stoi(optarg);
            if (timeout < 0)
            {
                cerr << "timeout must be a positive integer" << endl;
                exit(1);
            }
            break;

        case 'e':
            break;

        default:
            cerr << "Usage: " << argv[0] << "[-i TCPS<port>|UDPS<port>] [-o TCPC<adress,port>|UDPC<adress,port>] [-b TCPS<port>|UDPS<port>] [-t <timeout>]";
            exit(1);
            break;
        }
    }

    if ((input_flag || output_flag) && both_flag)
    {
        cerr << "-b cannot be used with -i or -o" << endl;
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
    int timeout = -1;
    string input;
    string output;
    string both;
    bool input_flag = false;
    bool output_flag = false;
    bool both_flag = false;
    OutputInfo output_struct;
    InputInfo input_struct;
    proccessArgs(argc, argv, opt, timeout, input, output, both, input_flag, output_flag, both_flag);

    if (input_flag && output_flag)
    {
        input_struct = extract_input_info(input);
        output_struct = extract_output_info(output);
        if ((input_struct.type == "TCPS" || input_struct.type == "TCPMUXS") && output_struct.type == "TCPC")
        {
            int client_socket = startTCPC(output_struct.address, output_struct.port);
            int server_socket;
            if (input_struct.type == "TCPS")
            {
                server_socket = startTCPS(input_struct.port);
                executeProgram(program, args, input_struct.type, server_socket, client_socket, false);
            }
            else if (input_struct.type == "TCPMUXS")
            {
                startTCPMUXS(input_struct.port, program, args, client_socket, false);
            }
        }
        else if (input_struct.type == "UDPS" && output_struct.type == "UDPC")
        {
            int client_socket = startUDPC(output_struct.address, output_struct.port);
            int server_socket = startUDPS(input_struct.port, timeout);
            executeProgram(program, args, input_struct.type, server_socket, client_socket, false);
        }
        else if (input_struct.type == "UDPS" && output_struct.type == "TCPC")
        {
            int client_socket = startTCPC(output_struct.address, output_struct.port);
            int server_socket = startUDPS(input_struct.port, timeout);
            executeProgram(program, args, input_struct.type, server_socket, client_socket, false);
        }
        else if ((input_struct.type == "TCPS" || input_struct.type == "TCPMUXS") && output_struct.type == "UDPC")
        {
            int client_socket = startUDPC(output_struct.address, output_struct.port);
            int server_socket;

            if (input_struct.type == "TCPS")
            {
                server_socket = startTCPS(input_struct.port);
                executeProgram(program, args, input_struct.type, server_socket, client_socket, false);
            }
            else if (input_struct.type == "TCPMUXS")
            {
                startTCPMUXS(input_struct.port, program, args, client_socket, false);
            }
        }
    }
    else if (input_flag)
    {
        input_struct = extract_input_info(input);

        int server_socket;
        if (input_struct.type == "TCPS")
        {
            server_socket = startTCPS(input_struct.port);
            executeProgram(program, args, input_struct.type, server_socket, -1, false);
        }
        else if (input_struct.type == "TCPMUXS")
        {
            startTCPMUXS(input_struct.port, program, args, -1, false);
        }
        else if (input_struct.type == "UDPS")
        {
            server_socket = startUDPS(input_struct.port, timeout);
            executeProgram(program, args, input_struct.type, server_socket, -1, false);
        }
    }
    else if (output_flag)
    {
        output_struct = extract_output_info(output);
        int client_socket = -1;
        if (output_struct.type == "TCPC")
        {
            client_socket = startTCPC(output_struct.address, output_struct.port);
        }
        else if (output_struct.type == "UDPC")
        {
            client_socket = startUDPC(output_struct.address, output_struct.port);
        }
        executeProgram(program, args, input_struct.type, -1, client_socket, false);
    }
    else if (both_flag)
    {
        input_struct = extract_input_info(both);
        int client_socket;
        if (input_struct.type == "TCPS")
        {
            client_socket = startTCPS(input_struct.port);
            executeProgram(program, args, input_struct.type, client_socket, client_socket, true);
        }
        else if (input_struct.type == "TCPMUXS")
        {
            startTCPMUXS(input_struct.port, program, args, -2, true);
        }
        else if (input_struct.type == "UDPS")
        {
            int sockfd = startUDPS(input_struct.port, timeout);
            executeProgram(program, args, input_struct.type, sockfd, sockfd, true);
        }
    }

    return 0;
}