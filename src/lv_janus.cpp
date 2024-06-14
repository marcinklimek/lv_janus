#include "stdafx.h"
#include "lv_janus.h"

#include <spdlog/spdlog.h>

#include "JanusC.h"

lv_janus::lv_janus()
{
    spdlog::info("lv_janus");
}

lv_janus::~lv_janus()
{
    spdlog::info("~lv_janus");
}

bool lv_janus::init()
{
    spdlog::info("init");

    auto ret = janusc_init();

    spdlog::info("janusC::main returned = {}", ret);

    return ret == 42;
}

void lv_janus::destroy()
{
    spdlog::info("destroy");

    janusc_destroy();
}

void lv_janus::send_receive(uint8_t * out_buf, int32_t out_size, uint8_t * in_buf, int32_t in_size)
{
    spdlog::info("send_receive out:({})[{}] in:({})[{}]", fmt::ptr(out_buf), out_size, fmt::ptr(in_buf), in_size);

    size_t out_len = out_size;
    size_t in_len = in_size;
    
    if (out_len != 0) 
    {
        spdlog::error("out len is 0");
    }
    
    if ( in_len != 0 ) 
    {
        spdlog::error("in len is 0");
    }
}

void lv_janus::dump_info()
{
    spdlog::info("Dumping info {}", 42);
}
