#pragma once
#include <string>

class ezUI {
public:
    static void dbg(const std::string& message);
};

#ifdef _DEBUG
#define EZUI_DEBUG 1
#else
#define EZUI_DEBUG 0
#endif