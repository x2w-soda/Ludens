#include <Ludens/CommandLine/ArgParser.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Memory/Allocator.h>

#include <algorithm>
#include <cstring>

namespace LD {

/// @brief each call to ArgParser::getopt consumes one result
struct ArgParseResult
{
    const char* payload;
    int index;
    ArgResult err;
};

struct ArgParserObj
{
    std::vector<ArgOption> options;
    std::vector<ArgParseResult> results;
    std::vector<LinearAllocator> allocators;
    size_t resultCtr;
    size_t positionalArgCtr;

    bool parse_arg(const char* arg, char** option, size_t& optionLen, char** payload, size_t& payloadLen);
    char* strndup(const char* str, size_t n);
    void* allocate(size_t size);
};

// Parse a single C string in argv. Handle 3 cases:
// 1. both option and payload present in the form of "--option=payload"
// 2. only option string in the form of "--option" or "-o", return true if it is a short name
// 3. only payload string "payload"
bool ArgParserObj::parse_arg(const char* arg, char** option, size_t& optionLen, char** payload, size_t& payloadLen)
{
    const char* payloadPos = nullptr;
    bool isShortName = false;
    optionLen = 0;
    payloadLen = 0;
    *payload = nullptr;
    *option = nullptr;

    if (arg[0] == '=')
        return false; // illegal input

    if (arg[0] == '-')
        isShortName = true;

    int i;

    for (i = 0; arg[i] != '\0'; i++)
    {
        if (isShortName && i == 1 && arg[i] == '-')
            isShortName = false; // found a long name

        // handle "--option=payload" or "-o=payload"
        if (arg[i] == '=')
        {
            int adjust = (isShortName ? 1 : 2);
            optionLen = i - adjust;
            if (optionLen > 0)
                *option = this->strndup(arg + adjust, optionLen);

            payloadPos = arg + i + 1;

            payloadLen = strlen(payloadPos);
            if (payloadLen > 0)
                *payload = this->strndup(payloadPos, payloadLen);

            return isShortName;
        }
    }

    // handle "--option" or "-o"
    if (arg[0] == '-')
    {
        int adjust = (isShortName ? 1 : 2);
        optionLen = i - adjust;
        if (optionLen > 0)
            *option = this->strndup(arg + adjust, optionLen);

        return isShortName;
    }

    // handle "payload"
    payloadLen = i;
    if (payloadLen > 0)
        *payload = this->strndup(arg, payloadLen);

    return false;
}

char* ArgParserObj::strndup(const char* str, size_t n)
{
    char* dup = (char*)this->allocate(n + 1);
    strncpy(dup, str, n);
    dup[n] = '\0';

    return dup;
}

void* ArgParserObj::allocate(size_t size)
{
    if (allocators.empty() || allocators.back().remain() < size)
    {
        LinearAllocatorInfo allocatorI;
        allocatorI.usage = MEMORY_USAGE_MISC;
        allocatorI.capacity = 256;

        if (allocatorI.capacity < size)
            allocatorI.capacity = (size_t)next_power_of_two((uint32_t)size);

        allocators.push_back(LinearAllocator::create(allocatorI));
    }

    LinearAllocator la = allocators.back();
    LD_ASSERT(la.remain() >= size);
    return la.allocate(size);
}

ArgParser ArgParser::create(int optionCount, const ArgOption* options)
{
    ArgParserObj* obj = heap_new<ArgParserObj>(MEMORY_USAGE_MISC);
    obj->options.resize((size_t)optionCount);

    // assume parameters are transient, copy strings over
    for (int i = 0; i < optionCount; i++)
    {
        ArgOption& optionCopy = obj->options[i];
        optionCopy.index = options[i].index;
        optionCopy.shortName = nullptr;
        optionCopy.longName = nullptr;
        optionCopy.payload = options[i].payload;

        if (options[i].shortName)
        {
            optionCopy.shortName = obj->strndup(options[i].shortName, 1);
        }

        if (options[i].longName)
        {
            size_t len = strlen(options[i].longName);
            optionCopy.longName = obj->strndup(options[i].longName, len);
        }
    }

    return {obj};
}

void ArgParser::destroy(ArgParser parser)
{
    ArgParserObj* obj = parser;

    for (LinearAllocator la : obj->allocators)
    {
        la.free();
        LinearAllocator::destroy(la);
    }

    heap_delete<ArgParserObj>(obj);
}

void ArgParser::parse(int argc, const char** argv)
{
    // NOTE: This function should never fail and be bullet proof to all kinds of weird input.
    //       Emphasize on testing.
    mObj->results.clear();
    mObj->resultCtr = 0;
    mObj->positionalArgCtr = 0;

    // it is tempting to do mObj->allocator.free() here but that would
    // invalidate the strings copied during ArgParser::create.

    ArgParseResult result{};
    bool getNextOpt = true;
    bool getPayloads = false;
    int payloadCtr = 0;

    for (int i = 0; i < argc; i++)
    {
        const char* arg = argv[i];
        char* option;
        char* payload;
        size_t optionLen;
        size_t payloadLen;

        bool isShortName = mObj->parse_arg(arg, &option, optionLen, &payload, payloadLen);
        bool onlyPayload = !option && payload;
        bool onlyOption = option && !payload;

        if (!option && !payload)
            continue; // ignore illegal input

        if (getPayloads && !onlyPayload)
        {
            getPayloads = false;
            getNextOpt = true;
            const ArgOption& opt = mObj->options[result.index];
            if (opt.payload == ARG_PAYLOAD_REQUIRED && payloadCtr == 0)
            {
                // missing payload
                result.err = ARG_RESULT_ERROR_MISSING_PAYLOAD;
                result.payload = nullptr;
                mObj->results.push_back(result);
                result = {.index = -1};
                getPayloads = false;
                getNextOpt = true;
                // fall through
            }
        }
        else if (getPayloads && onlyPayload)
        {
            result.payload = payload;
            result.err = (ArgResult)-1;
            mObj->results.push_back(result);
            result.payload = nullptr;
            payloadCtr++;
            continue; // getPayloads remains true
        }

        if (getNextOpt)
        {
            if (onlyPayload)
            {
                // positional argument
                result.payload = payload;
                result.err = ARG_RESULT_POSITIONAL;
                result.index = mObj->positionalArgCtr++;
                mObj->results.push_back(result);
                result = {.index = -1};
                continue; // getNextOpt remains true
            }

            auto ite = std::find_if(mObj->options.begin(), mObj->options.end(), [&](const ArgOption& opt) -> bool {
                if (isShortName)
                    return opt.shortName && *opt.shortName == option[0];
                return opt.longName && !strncmp(opt.longName, option, optionLen);
            });

            if (ite == mObj->options.end())
            {
                // unknown option
                result.index = -1;
                result.err = ARG_RESULT_ERROR_UNKNOWN_OPTION;
                result.payload = option;
                mObj->results.push_back(result);
                result = {.index = -1};
                continue; // getNextOpt remains true
            }

            result.index = static_cast<int>(ite - mObj->options.begin());

            if (ite->payload == ARG_PAYLOAD_NONE)
            {
                result.err = (ArgResult)-1;
                mObj->results.push_back(result);
                result = {.index = -1};
                continue; // getNextOpt remains true
            }

            // get optional or required payload
            if (payload)
            {
                result.err = (ArgResult)-1;
                result.payload = payload;
                mObj->results.push_back(result);
                result = {.index = -1};
                // getNextOpt remains true
            }
            else
            {
                getNextOpt = false;
                getPayloads = true;
            }

            payloadCtr = 0;
        }
    }

    // handle last result
    if (result.index >= 0)
    {
        const ArgOption& opt = mObj->options[result.index];
        if (opt.payload == ARG_PAYLOAD_REQUIRED && payloadCtr == 0)
        {
            // missing payload
            result.err = ARG_RESULT_ERROR_MISSING_PAYLOAD;
            result.payload = nullptr;
            mObj->results.push_back(result);
        }
    }
}

int ArgParser::getopt(const char** payload, int& errIndex)
{
    if (mObj->resultCtr == (int)mObj->results.size())
        return ARG_RESULT_EOF;

    const ArgParseResult& result = mObj->results[mObj->resultCtr++];

    if (result.err == ARG_RESULT_POSITIONAL)
    {
        errIndex = result.index;
        *payload = result.payload;
        return result.err;
    }
    else if (result.err == ARG_RESULT_ERROR_UNKNOWN_OPTION)
    {
        errIndex = -1;
        *payload = result.payload;
        return result.err;
    }
    else if (result.err == ARG_RESULT_ERROR_MISSING_PAYLOAD)
    {
        errIndex = result.index;
        *payload = nullptr;
        return result.err;
    }

    errIndex = -1;
    *payload = result.payload;
    return result.index;
}

} // namespace LD
