#include <cctype>
#include <md4c.h>
#include "Core/Serialize/Include/MD.h"

#define LD_MARKDOWN_PAGE_SIZE 4096

namespace LD
{

// currently only parses ASCII
LD_STATIC_ASSERT(sizeof(MD_CHAR) == 1);

/// implement md4c parser callbacks
struct MD4C
{
    /// @brief invoke the md4c parser and construct a MDDocument
    /// @return zero on success
    int Parse(const char* input, size_t size, MDParser* parser)
    {
        MD_PARSER md4c;
        md4c.abi_version = 0;
        md4c.enter_block = &MD4C::OnEnterBlock;
        md4c.leave_block = &MD4C::OnLeaveBlock;
        md4c.enter_span = &MD4C::OnEnterSpan;
        md4c.leave_span = &MD4C::OnLeaveSpan;
        md4c.text = &MD4C::OnText;
        md4c.syntax = nullptr;
        md4c.debug_log = nullptr;

        return md_parse((const MD_CHAR*)input, (MD_SIZE)size, &md4c, (void*)parser);
    }

    static int OnEnterBlock(MD_BLOCKTYPE /*type*/, void* /*detail*/, void* /*userdata*/);
    static int OnLeaveBlock(MD_BLOCKTYPE /*type*/, void* /*detail*/, void* /*userdata*/);
    static int OnEnterSpan(MD_SPANTYPE /*type*/, void* /*detail*/, void* /*userdata*/);
    static int OnLeaveSpan(MD_SPANTYPE /*type*/, void* /*detail*/, void* /*userdata*/);
    static int OnText(MD_TEXTTYPE /*type*/, const MD_CHAR* /*text*/, MD_SIZE /*size*/, void* /*userdata*/);

    static inline int OnEnterBlockP(MDParser* parser);
    static inline int OnLeaveBlockP(MDParser* parser);
    static inline int OnEnterBlockH(MDParser* parser, const MD_BLOCK_H_DETAIL* detail);
    static inline int OnLeaveBlockH(MDParser* parser, const MD_BLOCK_H_DETAIL* detail);
    
    static inline int OnEnterSpanEm(MDParser* parser);
    static inline int OnLeaveSpanEm(MDParser* parser);
    static inline int OnEnterSpanStrong(MDParser* parser);
    static inline int OnLeaveSpanStrong(MDParser* parser);
    static inline int OnEnterSpanCode(MDParser* parser);
    static inline int OnLeaveSpanCode(MDParser* parser);
};

int MD4C::OnEnterBlock(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MDParser* parser = (MDParser*)userdata;

    // clang-format off
    switch (type)
    {
    case MD_BLOCK_DOC:  return 0;
    case MD_BLOCK_P:    return OnEnterBlockP(parser);
    case MD_BLOCK_H:    return OnEnterBlockH(parser, (MD_BLOCK_H_DETAIL*)detail);
    default:            break;
    }
    // clang-format on

    return -1;
}

int MD4C::OnLeaveBlock(MD_BLOCKTYPE type, void* detail, void* userdata)
{
    MDParser* parser = (MDParser*)userdata;

    // clang-format off
    switch (type)
    {
    case MD_BLOCK_DOC:  return 0;
    case MD_BLOCK_P:    return OnLeaveBlockP(parser);
    case MD_BLOCK_H:    return OnLeaveBlockH(parser, (MD_BLOCK_H_DETAIL*)detail);
    default:            break;
    }
    // clang-format on

    return -1;
}

int MD4C::OnEnterSpan(MD_SPANTYPE type, void* detail, void* userdata)
{
    MDParser* parser = (MDParser*)userdata;

    // clang-format off
    switch (type)
    {
    case MD_SPAN_EM:      return OnEnterSpanEm(parser);
    case MD_SPAN_STRONG:  return OnEnterSpanStrong(parser);
    case MD_SPAN_CODE:    return OnEnterSpanCode(parser);
    default:              break;
    }
    // clang-format on

    return -1;
}

int MD4C::OnLeaveSpan(MD_SPANTYPE type, void* detail, void* userdata)
{
    MDParser* parser = (MDParser*)userdata;

    // clang-format off
    switch (type)
    {
    case MD_SPAN_EM:      return OnLeaveSpanEm(parser);
    case MD_SPAN_STRONG:  return OnLeaveSpanStrong(parser);
    case MD_SPAN_CODE:    return OnLeaveSpanCode(parser);
    default:              break;
    }
    // clang-format on

    return -1;
}

int MD4C::OnText(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata)
{
    MDParser* parser = (MDParser*)userdata;
    MDDocument* doc = parser->mDocument;
    MDBlock* blk = parser->mEntered.Top();

    LD_DEBUG_ASSERT(blk);

    MDSpan* span = doc->AllocSpan(parser->mSpanFlags);
    span->Content = { size, text };
    blk->AppendSpan(span);
    
    return 0;
}

int MD4C::OnEnterBlockP(MDParser* parser)
{
    MDDocument* doc = parser->mDocument;
    MDBlock* parent = parser->mEntered.Top();
    MDBlock* p = doc->AllocBlock(MDBlockType::P, parent);

    parser->mEntered.Push(p);

    return 0;
}

int MD4C::OnLeaveBlockP(MDParser* parser)
{
    MDBlock* blk = parser->mEntered.Top();
    LD_DEBUG_ASSERT(blk && blk->GetType() == MDBlockType::P);
    
    blk->mParent->AppendChild(blk);

    parser->mEntered.Pop();
    return 0;
}

inline int MD4C::OnEnterBlockH(MDParser* parser, const MD_BLOCK_H_DETAIL* detail)
{
    MDDocument* doc = parser->mDocument;
    MDBlock* parent = parser->mEntered.Top();
    MDBlockH* h = (MDBlockH*)doc->AllocBlock(MDBlockType::H, parent);

    h->Level = (int)detail->level;
    parser->mEntered.Push((MDBlock*)h);

    return 0;
}

inline int MD4C::OnLeaveBlockH(MDParser* parser, const MD_BLOCK_H_DETAIL* detail)
{
    MDBlock* blk = parser->mEntered.Top();
    LD_DEBUG_ASSERT(blk && blk->GetType() == MDBlockType::H);

    blk->mParent->AppendChild(blk);

    parser->mEntered.Pop();
    return 0;
}

inline int MD4C::OnEnterSpanEm(MDParser* parser)
{
    parser->mSpanFlags |= MD_SPAN_EM_BIT;
    return 0;
}

inline int MD4C::OnLeaveSpanEm(MDParser* parser)
{
    parser->mSpanFlags &= ~MD_SPAN_EM_BIT;
    return 0;
}

inline int MD4C::OnEnterSpanStrong(MDParser* parser)
{
    parser->mSpanFlags |= MD_SPAN_STRONG_BIT;
    return 0;
}

inline int MD4C::OnLeaveSpanStrong(MDParser* parser)
{
    parser->mSpanFlags &= ~MD_SPAN_STRONG_BIT;
    return 0;
}

inline int MD4C::OnEnterSpanCode(MDParser* parser)
{
    parser->mSpanFlags |= MD_SPAN_CODE_BIT;
    return 0;
}

inline int MD4C::OnLeaveSpanCode(MDParser* parser)
{
    parser->mSpanFlags &= ~MD_SPAN_CODE_BIT;
    return 0;
}


void MDBlock::AppendChild(MDBlock* blk)
{
    LD_DEBUG_ASSERT(blk);

    blk->mParent = this;
    
    if (mLastChild == nullptr)
        mFirstChild = mLastChild = blk;
    else
        mLastChild = mLastChild->mNext = blk;
}

void MDBlock::AppendSpan(MDSpan* span)
{
    LD_DEBUG_ASSERT(span);

    if (mLastSpan == nullptr)
        mFirstSpan = mLastSpan = span;
    else
        mLastSpan = mLastSpan->Next = span;
}


MDParser::MDParser(const MDParserConfig& config) : mConfig(config)
{
}

MDParser::~MDParser()
{
}

Ref<MDDocument> MDParser::ParseString(const char* md, size_t size)
{
    Ref<MDDocument> doc = MakeRef<MDDocument>();

    mSpanFlags = 0;
    mDocument = doc.get();
    mEntered.Clear();
    mEntered.Push(mDocument->mRootBlock);

    MD4C md4c;
    int result = md4c.Parse(md, size, this);

    mDocument = nullptr;

    return doc;
}

MDDocument::MDDocument()
{
    mRootBlock = AllocBlock(MDBlockType::DOC, nullptr);
}

MDDocument::~MDDocument()
{
    for (StackAllocator page : mPages)
        page.Cleanup();
}

MDBlock* MDDocument::AllocBlock(MDBlockType type, MDBlock* parent)
{
    if (mPages.IsEmpty() || mPages.Back().FreeBytes() < sizeof(MDBlock))
    {
        mPages.PushBack({});
        StackAllocator& page = mPages.Back();
        page.Startup(LD_MARKDOWN_PAGE_SIZE);
    }

    MDBlock* blk = (MDBlock*)mPages.Back().Alloc(sizeof(MDBlock));
    LD_DEBUG_ASSERT(blk != nullptr);

    memset(blk, 0, sizeof(MDBlock));
    blk->mType = type;
    blk->mParent = parent;
    blk->mDocument = this;
    return blk;
}

MDSpan* MDDocument::AllocSpan(MDSpanFlags flags)
{
    if (mPages.IsEmpty() || mPages.Back().FreeBytes() < sizeof(MDSpan))
    {
        mPages.PushBack({});
        StackAllocator& page = mPages.Back();
        page.Startup(LD_MARKDOWN_PAGE_SIZE);
    }

    MDSpan* span = (MDSpan*)mPages.Back().Alloc(sizeof(MDSpan));
    LD_DEBUG_ASSERT(span != nullptr);

    memset(span, 0, sizeof(MDSpan));
    span->Flags = flags;
    return span;
}

} // namespace LD