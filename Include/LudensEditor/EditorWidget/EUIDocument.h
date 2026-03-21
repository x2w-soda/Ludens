#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/UI/Widget/UIScrollWidget.h>
#include <Ludens/UI/Widget/UITextWidget.h>
#include <LudensBuilder/DocumentBuilder/Document.h>

namespace LD {

struct EUIDocumentItemStorage
{
    DocumentItem* item;
    UITextStorage text;
};

struct EUIDocumentStorage
{
    Document document = {};
    UIScrollStorage scroll;
    Vector<EUIDocumentItemStorage> items;
    
    /// @brief Build storage from document.
    /// @param doc Document to display.
    void build(Document doc);
};

void eui_document(EUIDocumentStorage* storage);

} // namespace LD