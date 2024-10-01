#pragma once

#include <string>
#include <unordered_map>
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/View.h"

namespace LD {

/// a command line argument
struct CommandLineArg
{
    /// short name of the argument
    const char* ShortName = nullptr;

    /// full name of the argument
    const char* FullName = nullptr;

    /// description of the argmument
    const char* Help = nullptr;

    /// if not empty, a list of accepted constant values
    View<const char*> AcceptedValues;

    /// if not nullptr, an optional constant default value
    const char* Default = nullptr;

    /// if set to true, the argument does not take on values, the Default and Values fields are ignored.
    bool IsFlag = false;

    /// if set to true, the argument values must be provided, or else parsing fails
    bool IsRequired = false;

    /// if set to true, the argument must be specified before or after other positional arguments
    bool IsPositional = false;

    const char* GetName() const
    {
        const char* name = FullName ? FullName : ShortName;
        LD_DEBUG_ASSERT(name != nullptr);
        return name;
    }
};

enum class CommandLineResultType
{
    /// the invocation is valid
    Ok = 0,

    /// the value does not belong to any argument
    UnknownValue,

    /// an argument does not accept the supplied input value
    UnacceptedValue,

    /// a required argument is not set
    RequiredArgumentMissing,
};

struct CommandLineResult
{
    CommandLineResultType Type;

    /// error message, if parsing fails
    std::string Error;

    inline operator bool() const
    {
        return Type == CommandLineResultType::Ok;
    }
};

/// command line argument parser
class CommandLineParser
{
public:
    /// @brief register an argument for the parser to recognize
    /// @return a non-negative value representing the argument
    /// @note There are a lot of debug assertions in this function that catches any ambiguity during
    ///       the creation of a parser. Proper runtime error handling is done only during Parse().
    int AddArgument(const CommandLineArg& arg);

    /// @brief parse command line arguments
    /// @return Ok on success, or returns an error enum and writes an error message string
    CommandLineResult Parse(int argc, const char** argv);

    /// @brief retrieve argument value after parsing
    /// @param arg the return value of some AddArgument call
    /// @param value the argument value, if present
    /// @return true if the argument is found during parsing, false otherwise
    bool GetArgument(int arg, std::string& value);

    /// @brief get a help string describing the usage of arguments
    /// @param invocation argv[0], or the name of the program
    /// @return a string describing how the program can be invoked
    std::string GetHelp(const char* invocation);

private:
    bool ParseWord(const char* word, CommandLineResult& result);
    bool CheckValue(int argi, CommandLineResult& result);

    bool SetArgument(int argi, const std::string& value, CommandLineResult& result);

    bool mDirty;
    int mCurrentArg;
    int mNextPositionalArg;
    std::string mHelp;
    std::unordered_map<int, std::string> mResult;
    Vector<CommandLineArg> mArgs;
};

} // namespace LD