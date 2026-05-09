#include <Ludens/Media/Format/MD.h>
#include <md4c.h>

namespace LD {

// static paranoia
static_assert((int)LD::MD_BLOCK_TYPE_DOC == (int)::MD_BLOCK_DOC);
static_assert((int)LD::MD_BLOCK_TYPE_QUOTE == (int)::MD_BLOCK_QUOTE);
static_assert((int)LD::MD_BLOCK_TYPE_UL == (int)::MD_BLOCK_UL);
static_assert((int)LD::MD_BLOCK_TYPE_OL == (int)::MD_BLOCK_OL);
static_assert((int)LD::MD_BLOCK_TYPE_LI == (int)::MD_BLOCK_LI);
static_assert((int)LD::MD_BLOCK_TYPE_HR == (int)::MD_BLOCK_HR);
static_assert((int)LD::MD_BLOCK_TYPE_H == (int)::MD_BLOCK_H);
static_assert((int)LD::MD_BLOCK_TYPE_CODE == (int)::MD_BLOCK_CODE);
static_assert((int)LD::MD_BLOCK_TYPE_HTML == (int)::MD_BLOCK_HTML);
static_assert((int)LD::MD_BLOCK_TYPE_P == (int)::MD_BLOCK_P);
static_assert((int)LD::MD_BLOCK_TYPE_TABLE == (int)::MD_BLOCK_TABLE);
static_assert((int)LD::MD_BLOCK_TYPE_THEAD == (int)::MD_BLOCK_THEAD);
static_assert((int)LD::MD_BLOCK_TYPE_TBODY == (int)::MD_BLOCK_TBODY);
static_assert((int)LD::MD_BLOCK_TYPE_TR == (int)::MD_BLOCK_TR);
static_assert((int)LD::MD_BLOCK_TYPE_TH == (int)::MD_BLOCK_TH);
static_assert((int)LD::MD_BLOCK_TYPE_TD == (int)::MD_BLOCK_TD);

static_assert((int)LD::MD_TEXT_TYPE_NORMAL == (int)::MD_TEXT_NORMAL);
static_assert((int)LD::MD_TEXT_TYPE_NULL_CHAR == (int)::MD_TEXT_NULLCHAR);
static_assert((int)LD::MD_TEXT_TYPE_BR == (int)::MD_TEXT_BR);
static_assert((int)LD::MD_TEXT_TYPE_SOFT_BR == (int)::MD_TEXT_SOFTBR);
static_assert((int)LD::MD_TEXT_TYPE_ENTITY == (int)::MD_TEXT_ENTITY);
static_assert((int)LD::MD_TEXT_TYPE_CODE == (int)::MD_TEXT_CODE);

static_assert((int)LD::MD_SPAN_TYPE_EM == (int)::MD_SPAN_EM);
static_assert((int)LD::MD_SPAN_TYPE_STRONG == (int)::MD_SPAN_STRONG);
static_assert((int)LD::MD_SPAN_TYPE_A == (int)::MD_SPAN_A);
static_assert((int)LD::MD_SPAN_TYPE_IMG == (int)::MD_SPAN_IMG);
static_assert((int)LD::MD_SPAN_TYPE_CODE == (int)::MD_SPAN_CODE);

static inline View view_md_attribute(const MD_ATTRIBUTE& attr)
{
    return View((const byte*)attr.text, (size_t)attr.size);
}

static inline MDBlockType get_native_block_type(MD_BLOCKTYPE inType)
{
    return (MDBlockType)inType;
}

static inline MDSpanType get_native_span_type(MD_SPANTYPE inType)
{
    return (MDSpanType)inType;
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
    case MD_BLOCK_TYPE_CODE:
        outDetail.code.lang = View((const byte*)static_cast<MD_BLOCK_CODE_DETAIL*>(inDetail)->lang.text,
                                   static_cast<MD_BLOCK_CODE_DETAIL*>(inDetail)->lang.size);
        outDetail.code.fenceChar = (char)static_cast<MD_BLOCK_CODE_DETAIL*>(inDetail)->fence_char;
        return true;
    default:
        break;
    }

    return false;
}

static inline bool get_native_span_detail(MDSpanType inType, void* inDetail, MDSpanDetail& outDetail)
{
    switch (inType)
    {
    case MD_SPAN_TYPE_A:
        outDetail.a.title = view_md_attribute(static_cast<MD_SPAN_A_DETAIL*>(inDetail)->title);
        outDetail.a.href = view_md_attribute(static_cast<MD_SPAN_A_DETAIL*>(inDetail)->href);
        return true;
    case MD_SPAN_TYPE_IMG:
        outDetail.img.title = view_md_attribute(static_cast<MD_SPAN_IMG_DETAIL*>(inDetail)->title);
        outDetail.img.src = view_md_attribute(static_cast<MD_SPAN_IMG_DETAIL*>(inDetail)->src);
        return true;
    default:
        break;
    }

    return false;
}

/// @brief Encapsulation of MD4C parser
class MD4CParser
{
public:
    MD4CParser() = delete;

    MD4CParser(const MDCallback& callbacks, void* user)
        : mCallbacks(callbacks), mUser(user)
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
        if (!self.mCallbacks.onEnterBlock)
            return 0;

        MDBlockDetail blkDetail;
        MDBlockType blkType = get_native_block_type(type);
        get_native_block_detail(blkType, detail, blkDetail);
        self.mCallbacks.onEnterBlock(blkType, blkDetail, self.mUser);
        return 0;
    }

    static int on_leave_block(MD_BLOCKTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mCallbacks.onLeaveBlock)
            return 0;

        MDBlockDetail blkDetail;
        MDBlockType blkType = get_native_block_type(type);
        get_native_block_detail(blkType, detail, blkDetail);
        self.mCallbacks.onLeaveBlock(blkType, blkDetail, self.mUser);
        return 0;
    }

    static int on_enter_span(MD_SPANTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mCallbacks.onEnterSpan)
            return 0;

        MDSpanDetail spanDetail;
        MDSpanType spanType = get_native_span_type(type);
        get_native_span_detail(spanType, detail, spanDetail);
        self.mCallbacks.onEnterSpan(spanType, spanDetail, self.mUser);
        return 0;
    }

    static int on_leave_span(MD_SPANTYPE type, void* detail, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mCallbacks.onLeaveSpan)
            return 0;

        MDSpanDetail spanDetail;
        MDSpanType spanType = get_native_span_type(type);
        get_native_span_detail(spanType, detail, spanDetail);
        self.mCallbacks.onLeaveSpan(spanType, spanDetail, self.mUser);
        return 0;
    }

    static int on_text(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* user)
    {
        MD4CParser& self = *(MD4CParser*)user;
        if (!self.mCallbacks.onText)
            return 0;

        MDTextType txtType = get_native_text_type(type);
        View txt((const byte*)text, (size_t)size);
        self.mCallbacks.onText(txtType, txt, self.mUser);
        return 0;
    }

private:
    MD_PARSER mMD4C;
    MDCallback mCallbacks;
    void* mUser;
};

bool MDParser::parse(View file, String& error, const MDCallback& callbacks, void* user)
{
    MD4CParser parser(callbacks, user);

    return parser.parse((const char*)file.data, file.size) == 0;
}

} // namespace LD
