#pragma once

#include <Extra/doctest/doctest.h>
#include <Ludens/CommandLine/ArgParser.h>

using namespace LD;

TEST_CASE("ArgParser ARG_RESULT_ERROR_UNKNOWN_OPTION")
{
    ArgParser parser = ArgParser::create(0, nullptr);
    {
        std::vector<const char*> args{"-h", "--help", "--verbose"};
        parser.parse((int)args.size(), args.data());
    }

    int ctr = 0;

    int errIndex, index;
    const char* payload;
    while ((index = parser.getopt(&payload, errIndex)) != ARG_RESULT_EOF)
    {
        CHECK(index == ARG_RESULT_ERROR_UNKNOWN_OPTION);
        switch (ctr)
        {
        case 0:
            CHECK(!strcmp(payload, "h"));
            break;
        case 1:
            CHECK(!strcmp(payload, "help"));
            break;
        case 2:
            CHECK(!strcmp(payload, "verbose"));
            break;
        }
        ctr++;
    }

    // 3 unknown options
    CHECK(ctr == 3);

    ArgParser::destroy(parser);
}

TEST_CASE("ArgParser ARG_RESULT_ERROR_MISSING_PAYLOAD")
{
    ArgParser parser;
    {
        std::vector<ArgOption> opts{
            {0, nullptr, "file", ARG_PAYLOAD_REQUIRED},
            {1, "f", nullptr, ARG_PAYLOAD_REQUIRED},
        };
        parser = ArgParser::create((int)opts.size(), opts.data());

        std::vector<const char*> args{"-f", "--file"};
        parser.parse(args.size(), args.data());
    }

    int ctr = 0;

    int errIndex, index;
    const char* payload;
    while ((index = parser.getopt(&payload, errIndex)) != ARG_RESULT_EOF)
    {
        if (ctr == 0)
        {
            CHECK(index == ARG_RESULT_ERROR_MISSING_PAYLOAD);
            CHECK(errIndex == 1); // -f missing payloads
        }
        else if (ctr == 1)
        {
            CHECK(index == ARG_RESULT_ERROR_MISSING_PAYLOAD);
            CHECK(errIndex == 0); // --file missing payloads
        }
        ctr++;
    }

    CHECK(ctr == 2);

    ArgParser::destroy(parser);
}

TEST_CASE("ArgParser ARG_RESULT_POSITIONAL")
{
    ArgParser parser;
    {
        std::vector<ArgOption> opts{
            {0, "f", "file", ARG_PAYLOAD_REQUIRED},
            {1, "h", "help", ARG_PAYLOAD_NONE},
        };
        parser = ArgParser::create((int)opts.size(), opts.data());
    }

    const char* posArg[3]{"pos0", "pos1", "pos2"};
    bool foundPosArg[3];

    int ctr = 0;
    int errIndex, index;
    const char* payload;
    foundPosArg[0] = foundPosArg[1] = foundPosArg[2] = false;

    // extract positional args between ARG_PAYLOAD_NONE
    {
        std::vector<const char*> args{posArg[0], "--help", posArg[1], "-h", posArg[2]};
        parser.parse(args.size(), args.data());

        while ((index = parser.getopt(&payload, errIndex)) != ARG_RESULT_EOF)
        {
            if (ctr == 0)
            {
                foundPosArg[0] = true;
                CHECK(!strcmp(posArg[0], payload));
            }
            else if (ctr == 2)
            {
                foundPosArg[1] = true;
                CHECK(!strcmp(posArg[1], payload));
            }
            else if (ctr == 4)
            {
                foundPosArg[2] = true;
                CHECK(!strcmp(posArg[2], payload));
            }

            ctr++;
        }

        CHECK(ctr == 5);
        CHECK(foundPosArg[0]);
        CHECK(foundPosArg[1]);
        CHECK(foundPosArg[2]);
    }

    ctr = 0;
    foundPosArg[0] = foundPosArg[1] = foundPosArg[2] = false;

    ArgParser::destroy(parser);
}

TEST_CASE("ArgParser short options")
{
    ArgParser parser;
    {
        std::vector<ArgOption> opts{
            {0, "h", nullptr, ARG_PAYLOAD_NONE},
            {1, "f", nullptr, ARG_PAYLOAD_REQUIRED},
        };
        parser = ArgParser::create((int)opts.size(), opts.data());
    }

    int ctr = 0;
    bool foundIndex0 = false;
    bool foundIndex1 = false;
    int errIndex, index;
    const char* payload;

    // short options with 0 or 2 payloads
    {
        std::vector<const char*> args{"-h", "-f", "foo", "bar"};
        parser.parse(args.size(), args.data());

        while ((index = parser.getopt(&payload, errIndex)) != ARG_RESULT_EOF)
        {
            switch (index)
            {
            case 0: // -h
                foundIndex0 = true;
                CHECK(ctr == 0);
                break;
            case 1: // -f
                foundIndex1 = true;
                CHECK(payload);
                if (ctr == 1)
                    CHECK(!strncmp(payload, "foo", 3));
                else if (ctr == 2)
                    CHECK(!strncmp(payload, "bar", 3));
                break;
            }

            ctr++;
        }

        CHECK(foundIndex0);
        CHECK(foundIndex1);
        CHECK(ctr == 3);
    }

    ctr = 0;
    foundIndex1 = false;
    bool foundPos = false;

    // short options with 1 payload
    {
        std::vector<const char*> args{"-f=foo,bar", "tar"};
        parser.parse(args.size(), args.data());

        while ((index = parser.getopt(&payload, errIndex)) != ARG_RESULT_EOF)
        {
            if (ctr == 0)
            {
                foundIndex1 = true;
                CHECK(index == 1);                  // -f option index
                CHECK(!strcmp(payload, "foo,bar")); // parse as single payload because it's after '='
            }
            if (ctr == 1)
            {
                foundPos = true;
                CHECK(index == ARG_RESULT_POSITIONAL);
                CHECK(errIndex == 0);           // position 0
                CHECK(!strcmp(payload, "tar")); // parse as positional arg because previous option captures with '='
            }

            ctr++;
        }

        CHECK(foundIndex1);
        CHECK(foundPos);
        CHECK(ctr == 2);
    }

    ArgParser::destroy(parser);
}