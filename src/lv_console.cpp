#include "stdafx.h"

#include "lv_console.h"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#define SHOW_CONSOLE 1

console_manager::console_manager()
{
#if SHOW_CONSOLE
    create_console();
#endif
}

console_manager::~console_manager()
{
#if SHOW_CONSOLE
    close_console();
#endif
}

void console_manager::create_console()
{
#if SHOW_CONSOLE

    if (!AllocConsole())
    {
        auto error_code = GetLastError();
        throw std::runtime_error("Failed to allocate console. Error code: " + std::to_string(error_code));
    }

    FILE* fDummy;
    errno_t ret_out = freopen_s(&fDummy, "CONOUT$", "w", stdout);
    errno_t ret_err = freopen_s(&fDummy, "CONOUT$", "w", stderr);
    errno_t ret_in = freopen_s(&fDummy, "CONIN$", "r", stdin);

    if (ret_out != 0 || ret_err != 0 || ret_in != 0)
    {
        auto error_code = GetLastError();
        FreeConsole();
        
        throw std::runtime_error("Failed to redirect streams. Error code: " + std::to_string(error_code));
    }

    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hConOut == INVALID_HANDLE_VALUE)
    {
        auto error_code = GetLastError();
        FreeConsole(); 
        throw std::runtime_error("Failed to create console output handle. Error code: " + std::to_string(error_code));
    }

    HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hConIn == INVALID_HANDLE_VALUE)
    {
        auto error_code = GetLastError();
        CloseHandle(hConOut); 
        FreeConsole(); 
        throw std::runtime_error("Failed to create console input handle. Error code: " + std::to_string(error_code));
    }

    if (!SetStdHandle(STD_OUTPUT_HANDLE, hConOut) || !SetStdHandle(STD_ERROR_HANDLE, hConOut) || !SetStdHandle(STD_INPUT_HANDLE, hConIn))
    {
        auto error_code = GetLastError();
        CloseHandle(hConOut);
        CloseHandle(hConIn);
        FreeConsole();
        throw std::runtime_error("Failed to set standard handles. Error code: " + std::to_string(error_code));
    }

    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();

#endif
}

void close_after_delay(HWND consoleWindow)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (consoleWindow)
        PostMessage(consoleWindow, WM_CLOSE, 0, 0);
}

void console_manager::close_console()
{
#if SHOW_CONSOLE
    HWND handle = GetConsoleWindow();

    if (!FreeConsole())
    {
        auto error_code = GetLastError();
        throw std::runtime_error("Failed to free console. Error code: " + std::to_string(error_code));
    }

    if (!SetStdHandle(STD_OUTPUT_HANDLE, nullptr) ||
        !SetStdHandle(STD_ERROR_HANDLE, nullptr) ||
        !SetStdHandle(STD_INPUT_HANDLE, nullptr))
    {
        auto error_code = GetLastError();
        throw std::runtime_error("Failed to reset standard handles. Error code: " + std::to_string(error_code));
    }
    
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();

    AttachConsole(ATTACH_PARENT_PROCESS);

    std::thread closeThread(close_after_delay, handle);  // stupid way to close the console window, it has to be done after a short delay
    closeThread.detach();

#endif
}
