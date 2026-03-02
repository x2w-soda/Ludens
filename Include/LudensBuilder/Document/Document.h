#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/View.h>

#include <string>

namespace LD {

struct Document : Handle<struct DocumentObj>
{
    static Document create(const View& md);
    void destroy(Document doc);

    std::string print();
};

} // namespace LD