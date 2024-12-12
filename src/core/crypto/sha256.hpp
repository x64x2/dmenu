#pragma once

#include <iostream>
#include <string>

namespace laiin {

namespace crypto {
    std::string sha256(const std::string& plain_text);
}

}
