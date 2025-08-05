#include <Ludens/Media/Format/MD.h>
#include <md4c.h>

namespace LD {

// static paranoia
static_assert(LD::MD_BLOCK_TYPE_DOC == ::MD_BLOCK_DOC);
static_assert(LD::MD_BLOCK_TYPE_QUOTE == ::MD_BLOCK_QUOTE);
static_assert(LD::MD_BLOCK_TYPE_UL == ::MD_BLOCK_UL);
static_assert(LD::MD_BLOCK_TYPE_OL == ::MD_BLOCK_OL);
static_assert(LD::MD_BLOCK_TYPE_LI == ::MD_BLOCK_LI);
static_assert(LD::MD_BLOCK_TYPE_HR == ::MD_BLOCK_HR);
static_assert(LD::MD_BLOCK_TYPE_H == ::MD_BLOCK_H);
static_assert(LD::MD_BLOCK_TYPE_CODE == ::MD_BLOCK_CODE);
static_assert(LD::MD_BLOCK_TYPE_HTML == ::MD_BLOCK_HTML);
static_assert(LD::MD_BLOCK_TYPE_P == ::MD_BLOCK_P);
static_assert(LD::MD_BLOCK_TYPE_TABLE == ::MD_BLOCK_TABLE);
static_assert(LD::MD_BLOCK_TYPE_THEAD == ::MD_BLOCK_THEAD);
static_assert(LD::MD_BLOCK_TYPE_TBODY == ::MD_BLOCK_TBODY);
static_assert(LD::MD_BLOCK_TYPE_TR == ::MD_BLOCK_TR);
static_assert(LD::MD_BLOCK_TYPE_TH == ::MD_BLOCK_TH);
static_assert(LD::MD_BLOCK_TYPE_TD == ::MD_BLOCK_TD);
static_assert(LD::MD_TEXT_TYPE_NORMAL == ::MD_TEXT_NORMAL);

static inline MDBlockType get_native_block_type(MD_BLOCKTYPE inType)
{
    return (MDBlockType)inType;
}

static inline MDTextType get_native_text_type(MD_TEXTTYPE inType)
{
    return (MDTextType)inType;
}

static inline bool get_native_block_detail(MDBlockType inType, void* inDetail, MDBlockDetail& outDetail)
{
    switch (inType)
    {
    case MD_BLOCK_TYPE_DOC:
        return true;
    case MD_BLOCK_TYPE_UL:
        outDetail.ul.isTight = static_cast<MD_BLOCK_UL_DETAIL*>(inDetail)->is_tight;
        outDetail.ul.mark = static_cast<MD_BLOCK_UL_DETAIL*>(inDetail)->mark;
        return true;
    case MD_BLOCK_TYPE_OL:
        outDetail.ol.start = static_cast<MD_BLOCK_OL_DETAIL*>(inDetail)->start;
        outDetail.ol.isTight = static_cast<MD_BLOCK_OL_DETAIL*>(inDetail)->is_tight;
        outDetail.ol.markDelimiter = static_cast<MD_BLOCK_OL_DETAIL*>(inDetail)->mark_delimiter;
        return true;
    case MD_BLOCK_TYPE_LI:
        outDetail.li.isTask = static_cast<MD_BLOCK_LI_DETAIL*>(inDetail)->is_task;
        outDetail.li.taskMark = static_cast<MD_BLOCK_LI_DETAIL*>(inDetail)->task_mark;
        outDetail.li.taskMarkOffset = static_cast<MD_BLOCK_LI_DETAIL*>(inDetail)->task_mark_offset;
        return true;
    case MD_BLOCK_TYPE_H:
        outDetail.h.level = static_cast<MD_BLOCK_H_DETAIL*>(inDetail)->level;
        return true;
    }

    return false;
}

/// @brief Encapsulation of MD4C parser
class MD4CParser
{
public:
    MD4CParser() = delete;

    MD4CParser(const MDEventParser& eventParser)
        : mEvents(eventParser)
    {
        mMD4C.abi_version = 0;
        mMD4C.flags = 0;
        mMD4C.debug_log = nullptr;
        mMD4C.enter_block = &MD4CParser::on_enter_block;
        mMD4C.leave_block = &MD4CParser::on_leave_block;
        mMD4C.enter_span = &MD4CParser::on_enter_span;
        mMD4C.leave_span = &MD4CParser::on_leave_span;
        mMD4C.text = &MD4CParser::on_text;
        mMD4C.debug_log = nullptr;
        mMD4C.syntax = nullptr;
    }

    int parse(const char* md, size_t size)
    {
        return ::md_parse((const MD_CHAR*)md, (MD_SIZE)size, &mMD4C, this);
    }

private:
    static int on_enter_block(MD_BLOCKTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mEvents.on_enter_block)
            return 0;

        MDBlockDetail blkDetail;
        MDBlockType blkType = get_native_block_type(type);
        get_native_block_detail(blkType, detail, blkDetail);
        self.mEvents.on_enter_block(blkType, blkDetail, self.mEvents.user);
        return 0;
    }

    static int on_leave_block(MD_BLOCKTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mEvents.on_leave_block)
            return 0;

        MDBlockDetail blkDetail;
        MDBlockType blkType = get_native_block_type(type);
        get_native_block_detail(blkType, detail, blkDetail);
        self.mEvents.on_leave_block(blkType, blkDetail, self.mEvents.user);
        return 0;
    }

    static int on_enter_span(MD_SPANTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        return 0;
    }

    static int on_leave_span(MD_SPANTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        return 0;
    }

    static int on_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mEvents.on_text)
            return 0;

        MDTextType txtType = get_native_text_type(type);
        MDString txt((const char*)text, (size_t)size);
        self.mEvents.on_text(txtType, txt, self.mEvents.user);
        return 0;
    }

private:
    MD_PARSER mMD4C;
    MDEventParser mEvents;
};

void MDDocument::parse_events(const char* md, size_t size, const MDEventParser& eventParser)
{
    MD4CParser parser(eventParser);
    parser.parse(md, size);
}

} // namespace LD