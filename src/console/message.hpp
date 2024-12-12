#pragma once

#include <iostream>
#include <string>

namespace laiin {

struct Message {
    std::string sender_id;
    std::string content;
    std::string recipient_id;
    std::string signature;
};

}
