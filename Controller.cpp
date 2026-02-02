#include "pch.h"
#include "exports.h"
#include <string>
#include <thread>
#include <cstddef>
std::thread Control;
static bool shouldBeRunning;
void ActualController() {
    const TCHAR* PIPE_NAME = TEXT("\\\\.\\pipe\\WINHVHOOK");

    char buffer[1024];
    DWORD dwRead;

    while (shouldBeRunning) {
        HANDLE hPipe = CreateNamedPipe(
            PIPE_NAME,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1,                  // max instances
            1024,               // out buffer
            1024,               // in buffer
            0,
            NULL
        );

        if (hPipe == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();

            if (err == ERROR_PIPE_BUSY) {
                WaitNamedPipe(PIPE_NAME, 2000);
                continue;
            }
            Sleep(1000);
            continue;
        }
        BOOL connected = ConnectNamedPipe(hPipe, NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected) {
            while (shouldBeRunning) {
                if (!ReadFile(hPipe, buffer, sizeof(buffer) - 1, &dwRead, NULL) || dwRead == 0)
                    break;

                buffer[dwRead] = '\0';

                if (buffer[0] == 's') {
                    std::string buff(buffer);
                    size_t pos = buff.find_first_of(' ');
                    std::string nums = buff.substr(pos+1);
                    UINT64 input = (UINT64)std::stoull(nums,nullptr,16);
                    
                    memscan(input,false);
                }
                //.text section of linker
                else if (buffer[0] == 't') {
                    ScanByteCodes();
                }
            }
        }
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
}
void Controller() {
    shouldBeRunning = true;
    Control = std::thread(ActualController);
}
void DeInitController() {
    shouldBeRunning = false;
    Control.join();
}