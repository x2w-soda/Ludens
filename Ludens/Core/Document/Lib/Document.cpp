#include "Core/Document/Include/Document.h"
#include "Core/Document/Include/DocumentLink.h"
#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/Control/Control.h"

#define LD_DOCUMENT_PADDING 60

namespace LD
{

Document::Document(const DocumentInfo& info)
{
    LD_DEBUG_ASSERT(info.Parent);

    mID = CUID<Document>::Get();
    mContext = info.Parent->GetContext();

    Rect2D windowRect;
    windowRect.x = 0.0f;
    windowRect.y = 0.0f;
    windowRect.w = info.Width;
    windowRect.h = info.Height;

    UIWindowInfo windowI;
    windowI.Context = mContext;
    windowI.Parent = info.Parent;
    windowI.Rect = windowRect;
    mWindow.Startup(windowI);
    mWindow.SetPadding(LD_DOCUMENT_PADDING, UIEdge::Top);
    mWindow.SetHPadding(LD_DOCUMENT_PADDING);

    UILabelInfo labelI;
    labelI.Widget.Parent = &mWindow;
    labelI.Text.Content = info.Title;
    labelI.Text.Font = info.Font;
    labelI.Text.Size = 60.0f;
    mTitle.Startup(labelI);
}

Document::~Document()
{
    mWindow.Cleanup();

    for (StackAllocator& page : mPages)
        page.Cleanup();
}

void Document::SetTitle(const char* title)
{
    mTitle.SetText(title);
}

UIWindow* Document::GetWindow()
{
    return &mWindow;
}

UID Document::GetID() const
{
    return mID;
}

} // namespace LD