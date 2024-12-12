#pragma once

#define laiin_TAG_OUT laiin::io_write("");
#define laiin_TAG_IN std::string("\033[1;35;49m[laiin]: \033[0m") +
#define laiin_TAG laiin_TAG_IN

#include <iostream>
#include <string>

namespace laiin {

    enum class log_priority {
        trace, error, warn, info
    };
    
    void log(log_priority priority, const std::string& message);
    
    void print(const std::string& text, int code = 0, bool log_msg = true); // 0=normal, 1=error, 2=warning, 3=success, 
    void io_write(const std::string& text); // like print but without a newline
}
