#pragma once

#include <string>

namespace LD
{

struct DocumentCompiler
{
    /// @brief compile doxygen XML to Markdown accepted by Reader
    /// @return true on successful compilation, false otherwise
    bool CompileDoxygenXML(const char* xml, std::string& md);
};

} // namespace LD