#pragma once

#include <Ludens/DSA/StringView.h>
#include <Ludens/Header/Handle.h>

namespace LD {

using MDString = StringView;

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

struct MDEventParser
{
    void* user; /// arbitrary user data propagated through events
    int (*on_enter_block)(MDBlockType type, const MDBlockDetail& detail, void* user);
    int (*on_leave_block)(MDBlockType type, const MDBlockDetail& detail, void* user);
    int (*on_text)(MDTextType type, const MDString& text, void* user);
};

struct MDDocument : Handle<struct MDDocumentObj>
{
    static void parse_events(const char* md, size_t size, const MDEventParser& eventParser);
};

} // namespace LD