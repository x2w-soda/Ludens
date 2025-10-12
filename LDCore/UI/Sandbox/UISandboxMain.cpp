#include "UISandbox.h"
#include <Ludens/Application/Application.h>
#include <Ludens/System/Memory.h>
#include <iostream>

using namespace LD;

int main(int argc, char** argv)
{
    {
        UISandbox sandbox;
        sandbox.run();
    }

    int count = LD::get_memory_leaks(nullptr);

    if (count > 0)
    {
        std::vector<LD::MemoryProfile> leaks(count);
        get_memory_leaks(leaks.data());

        for (int i = 0; i < count; i++)
            std::cout << "memory leak in usage " << get_memory_usage_cstr(leaks[i].usage) << ": " << leaks[i].current << " bytes" << std::endl;
    }

    return 0;
}