#include <doctest.h>

#include "Core/CommandLine/Include/CommandLine.h"
#include "Core/DSA/Include/Vector.h"

using namespace LD;

TEST_CASE("Flags")
{
    CommandLineParser parser;
    CommandLineResult result;

    CommandLineArg arg;
    arg.ShortName = "v";
    arg.FullName = "verbose";
    arg.Help = "print result in verbose";
    arg.IsFlag = true;
    int argVerbose = parser.AddArgument(arg);

    Vector<const char*> argv;
    std::string value, error;
    bool found;

    {
        argv = { "program", "-v" };
        result = parser.Parse(argv.Size(), argv.Data());
        found = parser.GetArgument(argVerbose, value);
        CHECK(result);
        CHECK(found);
    }

    {
        argv = { "program", "--verbose" };
        result = parser.Parse(argv.Size(), argv.Data());
        found = parser.GetArgument(argVerbose, value);
        CHECK(result);
        CHECK(found);
    }
}

TEST_CASE("Accepted Values")
{
    CommandLineParser parser;
    CommandLineResult result;

    const char* targetValues[2] = { "opengl", "vulkan" };
    CommandLineArg argTarget;
    argTarget.ShortName = "t";
    argTarget.FullName = "target";
    argTarget.Help = "the target backend to compile SPIRV shaders for";
    argTarget.AcceptedValues = { 2, targetValues };

    int target = parser.AddArgument(argTarget);

    Vector<const char*> argv;
    std::string value, error;
    bool found;

    // TODO:
    // the name is set, yet a value is not supplied
    {
        //argv = { "program", "-t" };
        //result = parser.Parse(argv.Size(), argv.Data());
        //CHECK(!result);
    }

    // a wrong value is supplied
    {
        argv = { "program", "--target", "dx12" };
        result = parser.Parse(argv.Size(), argv.Data());
        CHECK(!result);
        CHECK(result.Type == CommandLineResultType::UnacceptedValue);
    }

    // argument values are case sensitive
    {
        argv = { "program", "--target", "OpenGL" };
        result = parser.Parse(argv.Size(), argv.Data());
        CHECK(!result);
        CHECK(result.Type == CommandLineResultType::UnacceptedValue);
    }

    // accepted cases
    {
        argv = { "program", "--target", "opengl" };
        result = parser.Parse(argv.Size(), argv.Data());
        found = parser.GetArgument(target, value);
        CHECK(result);
        CHECK(found);
        CHECK(value == "opengl");

        argv = { "program", "-t", "vulkan" };
        result = parser.Parse(argv.Size(), argv.Data());
        found = parser.GetArgument(target, value);
        CHECK(result);
        CHECK(found);
        CHECK(value == "vulkan");
    }
}

TEST_CASE("Required Positional Accepeted Values")
{
    CommandLineParser parser;
    CommandLineResult result;

    Vector<const char*> values = { "shaderc", "assetc" };

    CommandLineArg argMode;
    argMode.FullName = "Mode";
    argMode.Help = "specify utility compiler mode";
    argMode.IsRequired = true;
    argMode.IsPositional = true;
    argMode.AcceptedValues = values.GetView();

    int argModeI = parser.AddArgument(argMode);

    Vector<const char*> argv;
    std::string value, error;
    bool found;

    // did not supply a value
    {
        argv = { "program" };

        result = parser.Parse(argv.Size(), argv.Data());
        CHECK(!result);
        CHECK(result.Type == CommandLineResultType::RequiredArgumentMissing);
    }

    // supplied a value, but not accepted
    {
        argv = { "program", "documentc" };

        result = parser.Parse(argv.Size(), argv.Data());
        CHECK(!result);
        CHECK(result.Type == CommandLineResultType::UnacceptedValue);
    }

    // supplied an accepted value
    {
        argv = { "program", "assetc" };

        result = parser.Parse(argv.Size(), argv.Data());
        found = parser.GetArgument(argModeI, value);
        CHECK(result);
        CHECK(found);
        CHECK(value == "assetc");
    }
}