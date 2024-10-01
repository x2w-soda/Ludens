#include <sstream>
#include "Core/CommandLine/Include/CommandLine.h"

namespace LD {

int CommandLineParser::AddArgument(const CommandLineArg& arg)
{
    mDirty = true;

    // at least supply one name
    LD_DEBUG_ASSERT(arg.ShortName || arg.FullName);

    // argument help description is mandatory
    LD_DEBUG_ASSERT(arg.Help);

    int token = (int)mArgs.Size();
    mArgs.PushBack(arg);

    return token;
}

CommandLineResult CommandLineParser::Parse(int argc, const char** argv)
{
    CommandLineResult result;
    result.Type = CommandLineResultType::Ok;

    mNextPositionalArg = -1;

    for (size_t argi = 0; argi < mArgs.Size(); argi++)
        if (mArgs[argi].IsPositional)
            mNextPositionalArg = argi;

    mCurrentArg = mNextPositionalArg;
    mResult.clear();

    for (int i = 1; i < argc; i++)
    {
        if (!ParseWord(argv[i], result))
            return result;
    }

    // post parsing validation
    for (size_t argi = 0; argi < mArgs.Size(); argi++)
    {
        if (!CheckValue(argi, result))
            return result;
    }

    return result;
}

bool CommandLineParser::GetArgument(int arg, std::string& value)
{
    if (mResult.find(arg) == mResult.end())
        return false;

    value = mResult[arg];
    return true;
}

std::string CommandLineParser::GetHelp(const char* invocation)
{
    int maxNameWidth = 0;

    for (const CommandLineArg& arg : mArgs)
    {
        int nameWidth = 0;

        if (arg.ShortName)
            nameWidth += strlen(arg.ShortName);

        if (arg.FullName)
            nameWidth += strlen(arg.FullName);

        if (arg.ShortName && arg.FullName)
            nameWidth += 2; // separator ", "

        maxNameWidth = nameWidth > maxNameWidth ? nameWidth : maxNameWidth;
    }

    std::stringstream ss;
    std::string line;
    ss << "usage: " << invocation << '\n';

    for (const CommandLineArg& arg : mArgs)
    {
        line.clear();

        if (arg.ShortName)
            line += arg.ShortName;

        if (arg.ShortName && arg.FullName)
            line += ", ";

        if (arg.FullName)
            line += arg.FullName;

        int padding = 2;
        size_t spaceWidth = (size_t)maxNameWidth - line.size();
        spaceWidth += padding;
        ss << line << std::string(spaceWidth, ' ') << arg.Help << '\n';
    }

    mHelp = ss.str();

    return mHelp;
}

bool CommandLineParser::ParseWord(const char* word, CommandLineResult& result)
{
    // parse input argument value
    if (word[0] != '-')
    {
        if (mCurrentArg < 0)
        {
            result.Type = CommandLineResultType::UnknownValue;
            result.Error = "error: ";
            result.Error += "value \"";
            result.Error += word;
            result.Error += "\" does not belong to any argument\n";
            return false;
        }

        return SetArgument(mCurrentArg, word, result);
    }

    // parse input argument name
    bool isFullName = false;
    
    ++word;
    if (word[0] == '-')
    {
        ++word;
        isFullName = true;
    }

    for (size_t argi = 0; argi < mArgs.Size(); argi++)
    {
        const CommandLineArg& arg = mArgs[argi];
        const char* name = isFullName ? arg.FullName : arg.ShortName;
        size_t len = strlen(name);

        if (!strncmp(name, word, len))
        {
            if (arg.IsFlag)
            {
                // mark flag as set
                mResult[argi] = {};
                break;
            }

            // next word is the value of this arg
            mCurrentArg = argi;
            break;
        }
    }

    return true;
}

bool CommandLineParser::CheckValue(int argi, CommandLineResult& result)
{
    const CommandLineArg& arg = mArgs[argi];
    const char* argName = arg.GetName();

    std::string value;
    bool found = GetArgument(argi, value);

    // check for missing required values
    if (arg.IsRequired && !found)
    {
        result.Type = CommandLineResultType::RequiredArgumentMissing;
        result.Error = "error: ";
        result.Error += "required argument \"";
        result.Error += argName;
        result.Error += "\" is not set\n";
        return false;
    }

    return true;
}

bool CommandLineParser::SetArgument(int argi, const std::string& value, CommandLineResult& result)
{
    const CommandLineArg& arg = mArgs[argi];

    // the supplied value must be one of the accepted values
    if (arg.AcceptedValues.Size() > 0)
    {
        bool isAccepted = false;
        for (const char* acceptedValue : arg.AcceptedValues)
        {
            if (value == acceptedValue)
            {
                isAccepted = true;
                break;
            }
        }

        if (!isAccepted)
        {
            result.Type = CommandLineResultType::UnacceptedValue;
            result.Error = "error: ";
            result.Error += "argument \"";
            result.Error += arg.FullName;
            result.Error += "\" does not accept value \"";
            result.Error += value;
            result.Error += "\", accepted values are:\n";

            for (const char* acceptedValue : arg.AcceptedValues)
            {
                result.Error += "- ";
                result.Error += acceptedValue;
                result.Error += '\n';
            }

            return false;
        }
    }

    mResult[argi] = value;

    if (argi == mCurrentArg)
    {
        // TODO:
    }

    return true;
}

} // namespace LD