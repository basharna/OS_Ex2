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
#include <sys/un.h>

using namespace std;

pid_t childpid = -1;

struct OutputInfo
{
    string type;
    string address;
    int port;
    string path;
};

struct InputInfo
{
    string type;
    int port;
    string path;
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

int startUDSSD(string path)
{
    int sockfd;
    struct sockaddr_un address;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == 0)
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

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path.c_str());
    unlink(address.sun_path);
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cerr << "bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int startUDSCD(string path)
{
    int sockfd;
    struct sockaddr_un serv_addr;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
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

    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, path.c_str());

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "connect failed" << endl;
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int startUDSSS(string path)
{
    int sockfd;
    struct sockaddr_un address;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == 0)
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

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path.c_str());
    unlink(address.sun_path);
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cerr << "bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    if (listen(sockfd, 3) < 0)
    {
        cerr << "listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    int addrlen = sizeof(address);
    int new_socket;
    if ((new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
        cerr << "accept failed" << endl;
        exit(EXIT_FAILURE);
    }

    return new_socket;
}

int startUDSCS(string path)
{
    int sockfd;
    struct sockaddr_un serv_addr;

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
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

    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, path.c_str());

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
    if (input.find("UDSSD") != string::npos || input.find("UDSSS") != string::npos)
    {
        regex udss_regex("^(UDSSD|UDSSS)(.+)+$");
        return regex_match(input, udss_regex);
    }
    else
    {
        regex tcp_udp_regex("^(TCPS|UDPS)\\d+$");
        return regex_match(input, tcp_udp_regex);
    }
}

bool validate_output(string &output)
{
    if (output.find("UDSCD") != string::npos || output.find("UDSCS") != string::npos)
    {
        regex udsc_regex("^(UDSCD|UDSCS)(.+)+$");
        return regex_match(output, udsc_regex);
    }
    else
    {
        regex tcp_udp_regex("^(TCPC|UDPC)[^,]+,\\d+$");
        return regex_match(output, tcp_udp_regex);
    }
}

OutputInfo extract_output_info(const std::string &output)
{
    OutputInfo info;
    if (output.find("UDSCD") != string::npos || output.find("UDSCS") != string::npos)
    {

        regex pattern("^(UDSCD|UDSCS)(.+)+$");

        smatch match;
        if (regex_match(output, match, pattern))
        {
            info.type = match[1].str();
            info.path = match[2].str();
            info.port = -1;
            info.address = "";
        }
        return info;
    }
    else
    {
        regex pattern("^(TCPC|UDPC)([^,]+),([0-9]+)$");

        smatch match;
        if (regex_match(output, match, pattern))
        {
            info.type = match[1].str();
            info.address = match[2].str();
            info.port = stoi(match[3].str());
            info.path = "";
        }

        return info;
    }
}

InputInfo extract_input_info(const std::string &input)
{
    InputInfo info;

    if (input.find("UDSSD") != string::npos || input.find("UDSSS") != string::npos)
    {
        regex pattern("^(UDSSD|UDSSS)(.+)+$");

        smatch match;
        if (regex_match(input, match, pattern))
        {
            info.type = match[1].str();
            info.path = match[2].str();
            info.port = -1;
        }
        return info;
    }
    else
    {
        regex pattern("^(TCPS|UDPS)([0-9]+)$");

        smatch match;
        if (regex_match(input, match, pattern))
        {
            info.type = match[1].str();
            info.port = stoi(match[2].str());
            info.path = "";
        }
        return info;
    }
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
        int client_socket = -1;
        int server_socket = -1;
        if (input_struct.type == "TCPS" && output_struct.type == "TCPC")
        {
            client_socket = startTCPC(output_struct.address, output_struct.port);
            server_socket = startTCPS(input_struct.port);
        }
        else if (input_struct.type == "UDPS" && output_struct.type == "UDPC")
        {
            client_socket = startUDPC(output_struct.address, output_struct.port);
            server_socket = startUDPS(input_struct.port, timeout);
        }
        else if (input_struct.type == "UDPS" && output_struct.type == "TCPC")
        {
            client_socket = startTCPC(output_struct.address, output_struct.port);
            server_socket = startUDPS(input_struct.port, timeout);
        }
        else if (input_struct.type == "TCPS" && output_struct.type == "UDPC")
        {
            client_socket = startUDPC(output_struct.address, output_struct.port);
            server_socket = startTCPS(input_struct.port);
        }
        else if (input_struct.type == "UDSSD" && output_struct.type == "UDSCD")
        {
            client_socket = startUDSCD(output_struct.path);
            server_socket = startUDSSD(input_struct.path);
        }
        else if (input_struct.type == "UDSSD" && output_struct.type == "UDSCS")
        {
            client_socket = startUDSCS(output_struct.path);
            server_socket = startUDSSD(input_struct.path);
        }
        else if (input_struct.type == "UDSSS" && output_struct.type == "UDSCD")
        {
            client_socket = startUDSCD(output_struct.path);
            server_socket = startUDSSS(input_struct.path);
        }
        else if (input_struct.type == "UDSSS" && output_struct.type == "UDSCS")
        {
            client_socket = startUDSCS(output_struct.path);
            server_socket = startUDSSS(input_struct.path);
        }
        executeProgram(program, args, input_struct.type, server_socket, client_socket, false);
    }
    else if (input_flag)
    {
        cout << "input : " << input << endl;
        input_struct = extract_input_info(input);
        int server_socket = -1;
        if (input_struct.type == "TCPS")
        {
            server_socket = startTCPS(input_struct.port);
        }
        else if (input_struct.type == "UDPS")
        {
            server_socket = startUDPS(input_struct.port, timeout);
        }
        else if (input_struct.type == "UDSSD")
        {
            server_socket = startUDSSD(input_struct.path);
        }
        else if (input_struct.type == "UDSSS")
        {
            server_socket = startUDSSS(input_struct.path);
        }
        executeProgram(program, args, input_struct.type, server_socket, -1, false);
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
        else if (output_struct.type == "UDSCD")
        {
            client_socket = startUDSCD(output_struct.path);
        }
        else if (output_struct.type == "UDSCS")
        {
            client_socket = startUDSCS(output_struct.path);
        }

        executeProgram(program, args, input_struct.type, -1, client_socket, false);
    }
    else if (both_flag)
    {
        input_struct = extract_input_info(both);
        int socket = -1;
        if (input_struct.type == "TCPS")
        {
            socket = startTCPS(input_struct.port);
        }
        else if (input_struct.type == "UDPS")
        {
            socket = startUDPS(input_struct.port, timeout);
        }
        else if (input_struct.type == "UDSSD")
        {
            socket = startUDSSD(input_struct.path);
        }
        else if (input_struct.type == "UDSSS")
        {
            socket = startUDSSS(input_struct.path);
        }
        executeProgram(program, args, input_struct.type, socket, socket, true);
    }

    return 0;
}