#pragma once

#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/View.h"
#include "Core/DSA/Include/Stack.h"
#include "Core/OS/Include/Memory.h"
#include "Core/OS/Include/Allocator.h"

namespace LD
{

class MDDocument;
class MDBlock;
class MD4C;
struct MDSpan;

using MDString = View<char>;

//
// Markdown Spans
//

enum MDSpanBit
{
    // emphasis, commonly rendered as italic
    MD_SPAN_EM_BIT = 1,

    // strong, commonly rendered as bold
    MD_SPAN_STRONG_BIT = 2,

    // code inline span
    MD_SPAN_CODE_BIT = 4,
};

using MDSpanFlags = unsigned char;

/// text that can rendered in the same style
struct MDSpan
{
    MDSpanFlags Flags;
    MDString Content;
    MDSpan* Next;
};

//
// Markdown Blocks
//

enum class MDBlockType
{
    DOC = 0,      // document
    P,            // paragraph <p></p>
    H,            // heading <h1></h1> up to <h6></h6>
};

/// markdown heading block
struct MDBlockH
{
    int Level;
};

class MDBlock
{
    friend class MDDocument;
    friend class MDParser;
    friend class MD4C;

public:

    // safely cast to heading block, or nullptr
    inline MDBlockH* ToH()
    {
        return mType == MDBlockType::H ? (MDBlockH*)this : nullptr;
    }

    /// get block type
    inline MDBlockType GetType()
    {
        return mType;
    }

    /// get next sibling block, or nullptr
    inline MDBlock* GetNext()
    {
        return mNext;
    }

    /// get first child block, or nullptr
    inline MDBlock* GetChild()
    {
        return mFirstChild;
    }

    /// get first span in block
    inline MDSpan* GetSpans()
    {
        return mFirstSpan;
    }

private:
    void AppendChild(MDBlock* blk);
    void AppendSpan(MDSpan* span);

    // NOTE: this union must be the first class member, so that we can
    //       directly type cast 'this' pointer to its actual derived type
    union
    {
        MDBlockH H;
    };

    MDBlockType mType;
    MDDocument* mDocument;
    MDBlock* mNext;
    MDBlock* mParent;
    MDBlock* mFirstChild;
    MDBlock* mLastChild;
    MDSpan* mFirstSpan;
    MDSpan* mLastSpan;
};

//
// Markdown Document
//

struct MDParserConfig
{
};

class MDParser
{
    friend class MD4C;

public:
    MDParser() = delete;
    MDParser(const MDParserConfig& config);
    MDParser(const MDParser&) = delete;
    ~MDParser();

    MDParser& operator=(const MDParser&) = delete;

    Ref<MDDocument> ParseString(const char* md, size_t size);

private:
    Stack<MDBlock*> mEntered;
    MDSpanFlags mSpanFlags;
    MDDocument* mDocument;
    MDParserConfig mConfig;
};

class MDDocument
{
    friend class MDParser;
    friend class MD4C;

public:
    MDDocument();
    MDDocument(const MDDocument&) = delete;
    ~MDDocument();

    /// get document root block
    inline MDBlock* GetBlock()
    {
        return mRootBlock;
    }

private:
    MDBlock* AllocBlock(MDBlockType type, MDBlock* parent);
    MDSpan* AllocSpan(MDSpanFlags flags);

    Vector<StackAllocator> mPages;
    MDBlock* mRootBlock = nullptr;
};

} // namespace LD