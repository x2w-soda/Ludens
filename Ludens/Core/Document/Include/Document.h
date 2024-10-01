#pragma once

#include "Core/OS/Include/UID.h"
#include "Core/OS/Include/Allocator.h"
#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/Control/Control.h"

namespace LD {

class ReaderUI;

struct DocumentInfo
{
    UIWindow* Parent;   // parent window of the created document window
    UIFont* Font;
    const char* Title;
    float Width;
    float Height;
};

/// a single page of information to display.
class Document
{
    friend class ReaderUI;

public:
    Document() = delete;
    Document(const Document&) = delete;
    Document(const DocumentInfo& info);
    ~Document();

    Document& operator=(const Document&) = delete;

    void SetTitle(const char* title);

    UIWindow* GetWindow();

    /// get document unique identifier
    UID GetID() const;

    void AddText();

private:
    UID mID;
    UIContext* mContext;
    UIWindow mWindow;
    UILabel mTitle;
    Vector<StackAllocator> mPages;
};

} // namespace LD