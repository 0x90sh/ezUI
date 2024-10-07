#include "ezui.hpp"
#include <iostream>

void ezUI::dbg(const std::string& message) {
#if EZUI_DEBUG
    std::cout << "[dbg] " << message << std::endl;
#endif
}