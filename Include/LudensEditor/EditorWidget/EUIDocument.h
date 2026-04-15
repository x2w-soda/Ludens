#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/UI/Widget/UIScrollWidget.h>
#include <Ludens/UI/Widget/UITextWidget.h>
#include <LudensBuilder/DocumentBuilder/Document.h>
#include <LudensEditor/EditorWidget/EUIScroll.h>

namespace LD {

struct EUIDocumentItem
{
    DocumentItem* item;
    UITextData text;
};

class EUIDocument
{
public:
    /// @brief Build storage from document.
    /// @param doc Document to display.
    void build(Document doc);

    void push();
    void pop();

    bool get_request_uri_path(std::string& outPath);
    void set_request_uri_path(const std::string& path);

private:
    Document mDocument = {};
    EUIScroll mScroll;
    Vector<EUIDocumentItem> mItems;
    std::string mRequestURIPath = {}; // if not empty, user should rebuild from the requested URI
};

} // namespace LD