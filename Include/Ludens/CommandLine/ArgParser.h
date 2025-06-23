#pragma once

#include <Ludens/Header/Handle.h>

namespace LD {

enum ArgPayloadType : int
{
    ARG_PAYLOAD_NONE = 0, /// no payload is allowed (--help, --verbose)
    ARG_PAYLOAD_REQUIRED, /// required payload (--file=payload, --file payload, -f payload)
    ARG_PAYLOAD_OPTIONAL, /// optional payload (--color, --color=auto, --color auto)
};

enum ArgResult : int
{
    ARG_RESULT_EOF = -1,
    ARG_RESULT_POSITIONAL = -2,
    ARG_RESULT_ERROR_UNKNOWN_OPTION = -3,  /// unregistered option
    ARG_RESULT_ERROR_MISSING_PAYLOAD = -4, /// ARG_PAYLOAD_REQUIRED but not found
};

struct ArgOption
{
    int index;              /// non-negative index that identifies an option
    const char* shortName;  /// if not null, a single char as the option short name
    const char* longName;   /// if not null, a null-terminated C string as the option long name
    ArgPayloadType payload; /// payload restrictions
};

/// @brief Stand-alone command line argument parser similar to the POSIX getopt standard.
///        Short options and long options are both supported.
///        Positional arguments are supported.
struct ArgParser : Handle<struct ArgParserObj>
{
    /// @brief create argument parser
    /// @return parser handle
    static ArgParser create(int optionCount, const ArgOption* options);

    /// @brief destroy argument parser
    /// @param parser valid parser handle
    static void destroy(ArgParser parser);

    /// @brief parse arguments for user to call getopt()
    /// @param argc length of the argument vector array
    /// @param argv the argument vector to be parsed
    void parse(int argc, const char** argv);

    /// @brief get-option API similar to the POSIX getopt standard.
    ///        User repeatedly calls this function until it returns ARG_RESULT_EOF.
    /// @param payload Outputs a C string or nullptr, interpretation depends on function return value.
    /// @param errIndex Outputs an index, interpretation depends on function return value.
    /// @return The non-negative option index on success, otherwise a ArgResult enum.
    ///         On success, \p payload outputs the argument payload or null.
    ///         On ARG_RESULT_EOF, all options are already processed.
    ///         On ARG_RESULT_POSITIONAL, \p payload outputs the argument name and \p errIndex outputs its position.
    ///         On ARG_RESULT_ERROR_MISSING_PAYLOAD, \p errIndex is the argument with ARG_PAYLOAD_REQUIRED.
    ///         On ARG_RESULT_ERROR_UNKNOWN_OPTION, \p payload outputs the unregistered option name.
    int getopt(const char** payload, int& errIndex);
};

} // namespace LD