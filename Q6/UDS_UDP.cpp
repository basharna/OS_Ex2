#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

using namespace std;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <path>" << endl;
        exit(EXIT_FAILURE);
    }

    string path = argv[1];

    int sockfd;
    struct sockaddr_un address;

    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) == 0)
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

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, path.c_str());
    unlink(address.sun_path);
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cerr << "bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        char buffer[1024] = {0};
        recv(sockfd, buffer, 1024, 0);
        cout << buffer;
    }

    return 0;
}
