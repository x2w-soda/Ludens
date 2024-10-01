#include "Core/Document/Include/DocumentItem.h"

namespace LD
{

DocumentText::DocumentText(const char* md) : DocumentItem(DocumentItemType::Text)
{
    ParseMD(md);
}

DocumentText::~DocumentText()
{

}

void DocumentText::ParseMD(const char* md)
{
    // TODO:
}

} // namespace LD