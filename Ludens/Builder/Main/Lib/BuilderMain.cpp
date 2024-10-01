#include <cstdio>
#include <iostream>
#include "Builder/Main/Lib/Shaderc.h"
#include "Core/CommandLine/Include/CommandLine.h"

static void PrintUsage(const char* program)
{
    std::cout << "usage: " << program << "Mode" << std::endl;
    std::cout << "possible values for Mode are: shaderc" << std::endl;
}

int main(int argc, const char** argv)
{
    const char* modes[] = { "shaderc" };

    LD::CommandLineParser parser;
    LD::CommandLineResult result;
    
    if (argc == 1)
    {
        PrintUsage(argv[0]);
        return 0;
    }

    std::string mode(argv[1]);

    if (mode == "shaderc")
    {
        LD::Shaderc shaderc;
        return shaderc.Main(argc - 1, argv + 1);
    }
    else
    {
        std::cout << "unknown mode \"" << mode << "\"" << std::endl;
    }

    return 0;
}