#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    // Create a socket
    int socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1) {
        std::cerr << "Failed to create socket: " << strerror(errno)
                  << std::endl;
        return 1;
    }

    // Set up the server address
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(35500);
    server_address.sin_addr.s_addr =
        inet_addr("100.67.113.112");

    if (connect(socket_descriptor, (struct sockaddr *)&server_address,
                sizeof(server_address)) == -1) {
        std::cerr << "Failed to connect to server: " << strerror(errno)
                  << std::endl;
        close(socket_descriptor);
        return 1;
    }

    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(socket_descriptor, buffer, sizeof(buffer) - 1)) >
           0) {
        std::cout << "MOTION\n\n" << std::endl;
    }

    if (bytes_read == -1) {
        std::cerr << "Failed to read from server: " << strerror(errno)
                  << std::endl;
    }

    close(socket_descriptor);

    return 0;
}
