#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

constexpr int MAX_CLIENTS = 10;

struct client_ctx {
    int socket;
    std::string ipv4_address;
};

std::vector<client_ctx> clients;
std::mutex
    clients_mutex;

void signal_handler(int signum) {
    std::cout << "Received SIGTERM signal. Gracefully shutting down..."
              << std::endl;

    for (auto client : clients) {
        close(client.socket);
    }

    exit(signum);
}

void handle_client(int client_socket) {
    std::signal(SIGTERM, signal_handler);
    client_ctx client_context;
    client_context.socket = client_socket;

    int flag = 1;
    if (setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY,
                   reinterpret_cast<const char *>(&flag), sizeof(flag)) == -1) {
        perror("Failed to set TCP_NODELAY option");
    }
    int flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);

    if (getpeername(client_socket, (struct sockaddr *)&clientAddr,
                    &clientAddrLen) == 0) {
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        client_context.ipv4_address = std::string(clientIP);
    } else {
        perror("Failed to get client address");
        client_context.ipv4_address = "";
    }

    std::cout << "Accepted " << client_context.ipv4_address << std::endl;

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back(client_context);
    }

    fcntl(client_socket, F_SETFL, O_NONBLOCK);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create socket");
        return 1;
    }
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) < 0) {
        perror("Failed to set SO_REUSEADDR");
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(35500);

    if (bind(server_socket, reinterpret_cast<sockaddr *>(&server_address),
             sizeof(server_address)) == -1) {
        perror("Failed to bind socket");
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Failed to listen for connections");
        return 1;
    }

    std::thread accept_thread([&]() {
        while (true) {
            sockaddr_in client_address{};
            socklen_t client_address_length = sizeof(client_address);
            int client_socket = accept(
                server_socket, reinterpret_cast<sockaddr *>(&client_address),
                &client_address_length);
            if (client_socket == -1) {
                perror("Failed to accept connection");
            } else {
                std::thread client_thread(handle_client, client_socket);
                client_thread.detach(); 
                                        
            }
        }
    });


    std::ifstream serial("/dev/ttyACM0");
    if (!serial.is_open()) {
        std::cerr << "Failed to open Arduino device." << std::endl;
        return 1;
    }

    std::string line;
    bool waiting = false;
    const char z = 1;
    while (true) {
        std::getline(serial, line);
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            if (line == "1" && !waiting) {
                waiting = true;
                std::cout << "MOTION\n\n" << std::endl;
                for (auto it = clients.begin(); it != clients.end();) {
                    if (send(it->socket, &z, sizeof(z),
                             MSG_NOSIGNAL | MSG_DONTWAIT) <= 0) {
                        std::cout << it->ipv4_address << " disconnected"
                                  << std::endl;
                        it = clients.erase(it);
                    } else {
                        it++;
                    }
                }
            } else if (line == "0" && waiting) {
                waiting = false;
            }
        }
    }
    return 0;
}
