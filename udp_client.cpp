#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <SERVER_IP> <SERVER_PORT> <MESSAGE>" << std::endl;
        return 1;
    }

    std::string server_ip = argv[1];
    int server_port = std::atoi(argv[2]);
    std::string message = argv[3];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error opening UDP socket" << std::endl;
        return 1;
    }

    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    if (inet_aton(server_ip.c_str(), &serv_addr.sin_addr) == 0) {
        std::cerr << "Invalid server IP address" << std::endl;
        return 1;
    }

    if (sendto(sockfd, message.c_str(), message.length(), 0,
               (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error sending UDP message" << std::endl;
        return 1;
    }

    std::cout << "Sent UDP message: " << message << std::endl;

    close(sockfd);

    return 0;
}

