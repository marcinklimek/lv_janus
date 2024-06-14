#pragma once

#include <thread>

#include "lv_console.h"
#include "lv_logger.h"

class lv_janus
{

public:
    lv_janus();    
    ~lv_janus();

    // just for the sake of completeness
    lv_janus(const lv_janus& other) = delete;
    lv_janus(lv_janus&& other) noexcept = delete;
    lv_janus& operator=(const lv_janus& other) = delete;
    lv_janus& operator=(lv_janus&& other) noexcept = delete;

    bool init();
    void destroy();
    
    void send_receive(uint8_t* out_buf, int32_t out_size, uint8_t* in_buf, int32_t in_size);

    // debug
    void dump_info();

private:
    console_manager console;
    lv_logger logger;

    std::unique_ptr<std::thread> _th;
};


