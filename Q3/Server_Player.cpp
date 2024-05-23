#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int openSocket()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        cerr << "Error opening socket" << endl;
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void bindSocket(int sockfd, int port)
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "Error binding socket" << endl;
        exit(EXIT_FAILURE);
    }
}

void listenSocket(int sockfd)
{
    if (listen(sockfd, 5) < 0)
    {
        cerr << "Error listening on socket" << endl;
        exit(EXIT_FAILURE);
    }

}

int acceptSocket(int sockfd)
{
    int newsockfd = accept(sockfd, NULL, NULL);
    if (newsockfd < 0)
    {
        cerr << "Error accepting connection" << endl;
        exit(EXIT_FAILURE);
    }
    return newsockfd;
}

