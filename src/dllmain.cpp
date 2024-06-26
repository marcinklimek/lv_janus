#include "stdafx.h"
#include <stdint.h>

#include <cstdint>
#include <iostream>
#include <memory>

#include "lv_janus.h"

std::unique_ptr<lv_janus> janus;

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            std::cout << "DLL_PROCESS_DETACH" << std::endl;

        case DLL_PROCESS_DETACH :

            if (ul_reason_for_call == DLL_PROCESS_DETACH)
                std::cout << "DLL_PROCESS_DETACH" << std::endl;

            if (janus != nullptr)
            {
                janus->destroy();
                janus.reset();
                janus = nullptr;
            }

            break;

        default:;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) void __stdcall initialize()
{
    if (!janus)
    {
        janus = std::make_unique<lv_janus>();
        janus->init();
    }
}

extern "C" __declspec(dllexport) void __stdcall send_receive(uint8_t * out_buf, int32_t out_size, uint8_t * in_buf, int32_t in_size)
{
    if (janus)
        janus->send_receive(out_buf, out_size, in_buf, in_size);
}

extern "C" __declspec(dllexport) void __stdcall destroy()
{
    if (janus)
    {
        janus->destroy();
        janus.reset();
        janus = nullptr;
    }
}

extern "C" __declspec(dllexport) void __stdcall dump_info()
{
    if (janus)
        janus->dump_info();
}
