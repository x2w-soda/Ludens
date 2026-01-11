#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Memory.h>

#include "EditorApplication.h"

#include <iostream>

int main(int argc, char** argv)
{
    {
        LD::EditorApplication editorApp;
        editorApp.run();
    }

    int count = LD::get_memory_leaks(nullptr);

    if (count > 0)
    {
        LD::Vector<LD::MemoryProfile> leaks(count);
        LD::get_memory_leaks(leaks.data());

        for (int i = 0; i < count; i++)
            std::cout << "memory leak in usage " << LD::get_memory_usage_cstr(leaks[i].usage) << ": " << leaks[i].current << " bytes" << std::endl;
    }
}