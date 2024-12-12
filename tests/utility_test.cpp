#include <iostream>
#include <string>
// laiin
#include "../src/laiin.hpp"
using namespace laiin;


int main() {

    std::cout << laiin::string::precision("1.123456789", 9) << std::endl;
    std::cout << laiin::string::lower("LOWER") << std::endl;
    std::cout << laiin::string::upper("upper") << std::endl;
    
    #if defined(__cplusplus) && (__cplusplus >= 201402L) // 14
    std::cout << "This project is using C++ 14\n";
    #endif
    #if defined(__cplusplus) && (__cplusplus >= 201703L) // 17
    std::cout << "This project is using C++ 17\n";
    #endif
    std::cout << "current dir: " << laiin::filesystem::current_directory() << std::endl;
    std::cout << "mkdir result: " << laiin::filesystem::make_directory("dude") << std::endl;
    std::cout << "get_user result: " << laiin::device::get_user() << std::endl;
    std::cout << "filename result: " << laiin::filesystem::get_file_extension("player.png") << std::endl;
    
    return 0;
}
