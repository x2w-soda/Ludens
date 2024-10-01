#include <doctest.h>

#include "Core/CommandLine/Include/CommandLine.h"
#include "Core/DSA/Include/Vector.h"

using namespace LD;

TEST_CASE("flags")
{
    auto fuzz = [](Vector<const char*>& args) {
        CommandLineArg argFoo;
        argFoo.ShortName = "f";
        argFoo.FullName = "foo";
        argFoo.IsFlag = true;
        argFoo.Help = "flag foo";

        CommandLineArg argBar;
        argBar.ShortName = "b";
        argBar.FullName = "bar";
        argBar.IsFlag = true;
        argBar.Help = "flag bar";

        CommandLineParser parser;
        int argFooI = parser.AddArgument(argFoo);
        int argBarI = parser.AddArgument(argBar);

        int argc = (int)args.Size();
        const char** argv = args.Data();
        CommandLineResult result = parser.Parse(argc, argv);
        CHECK(result);

        std::string value;
        CHECK(parser.GetArgument(argFooI, value));
        CHECK(parser.GetArgument(argBarI, value));
    };

    Vector<const char*> args;

    args = { "program", "--foo", "--bar" };
    fuzz(args);

    args = { "program", "-b", "-f" };
    fuzz(args);

    // repeatedly setting one flag is equal to setting the flag once
    args = { "program", "-f", "--bar", "--foo", "-b" };
    fuzz(args);
}

TEST_CASE("positional arguments")
{
    auto fuzz = [](Vector<const char*>& args) {
        CommandLineArg argFile;
        argFile.FullName = "files";
        argFile.Help = "one or more input files";
        argFile.IsPositional = true;

        CommandLineArg argOutput;
        argOutput.ShortName = "o";
        argOutput.FullName = "output";
        argOutput.IsFlag = true;
        argOutput.Help = "output file name";

        CommandLineParser parser;
        int argFileI = parser.AddArgument(argFile);
        int argOutputI = parser.AddArgument(argOutput);

        int argc = (int)args.Size();
        const char** argv = args.Data();
        CommandLineResult result = parser.Parse(argc, argv);
        CHECK(result);

        std::string value;
        CHECK(parser.GetArgument(argFileI, value));
        CHECK(value == "main.c test.c");
        CHECK(parser.GetArgument(argOutputI, value));
        CHECK(value == "main.o");
    };

    Vector<const char*> args;

    // TODO: multiple positional values!

    args = { "program", "main.c", "test.c", "-o", "main.o" };
    fuzz(args);

    args = { "program", "-o", "main.o", "main.c", "test.c" };
    fuzz(args);

    // psychopath input, but still valid
    args = { "program", "main.c", "-o", "main.o", "test.c" };
    fuzz(args);
}