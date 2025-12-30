#pragma once

#include <Ludens/DSA/View.h>
#include <Ludens/Header/Handle.h>

#include <string>

namespace LD {

using MDString = View;

enum MDBlockType
{
    MD_BLOCK_TYPE_DOC = 0,
    MD_BLOCK_TYPE_QUOTE,
    MD_BLOCK_TYPE_UL,
    MD_BLOCK_TYPE_OL,
    MD_BLOCK_TYPE_LI,
    MD_BLOCK_TYPE_HR,
    MD_BLOCK_TYPE_H,
    MD_BLOCK_TYPE_CODE, // text lines within code blocks are terminated with '\n' instead of MD_TEXT_TYPE_BR
    MD_BLOCK_TYPE_HTML, // raw HTML block in verbatim
    MD_BLOCK_TYPE_P,
    MD_BLOCK_TYPE_TABLE,
    MD_BLOCK_TYPE_THEAD,
    MD_BLOCK_TYPE_TBODY,
    MD_BLOCK_TYPE_TR,
    MD_BLOCK_TYPE_TH,
    MD_BLOCK_TYPE_TD,
};

/// @brief Union of all possible detail structs of a markdown block
union MDBlockDetail
{
    struct UL
    {
        bool isTight; /// tight list or loose list
        char mark;    /// bullet character of the list. '-', '+', etc.
    } ul;

    struct OL
    {
        int start;          /// starting index of ordered list
        bool isTight;       /// tight list or loose list
        char markDelimiter; /// item delimiter character
    } ol;

    struct LI
    {
        bool isTask;
        char taskMark;
        int taskMarkOffset;
    } li;

    struct H
    {
        int level; /// header level 1-6
    } h;
};

enum MDTextType
{
    MD_TEXT_TYPE_NORMAL = 0,
};

struct MDEventCallback
{
    int (*onEnterBlock)(MDBlockType type, const MDBlockDetail& detail, void* user);
    int (*onLeaveBlock)(MDBlockType type, const MDBlockDetail& detail, void* user);
    int (*onText)(MDTextType type, const MDString& text, void* user);
};

struct MDEventParser
{
    static bool parse(const View& file, std::string& error, const MDEventCallback& callbacks, void* user);
};

} // namespace LD