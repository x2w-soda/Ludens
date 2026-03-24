#pragma once

#include <Ludens/Header/Range.h>
#include <Ludens/Header/View.h>

#include <string>

namespace LD {

/// @brief Uniform Resource Identifier
class URI
{
public:
    URI() = default;

    explicit URI(const char* cstr)
        : mString(cstr)
    {
        parse();
    }

    explicit URI(const std::string& str)
        : mString(str)
    {
        parse();
    }

    explicit URI(View view)
        : mString(view.data, view.size)
    {
        parse();
    }

    inline const std::string& string() const { return mString; }
    inline View view() const { return View(mString.data(), mString.size()); }

    inline View scheme() const { return View(mString.data() + mSchemeRange.offset, mSchemeRange.size); }
    inline View authority() const { return View(mString.data() + mAuthorityRange.offset, mAuthorityRange.size); }
    inline View path() const { return View(mString.data() + mPathRange.offset, mPathRange.size); }
    inline View query() const { return View(mString.data() + mQueryRange.offset, mQueryRange.size); }
    inline View fragment() const { return View(mString.data() + mFragmentRange.offset, mFragmentRange.size); }

private:
    void parse()
    {
        std::string_view remaining = mString;

        mSchemeRange = {};
        mAuthorityRange = {};
        mPathRange = {};
        mQueryRange = {};
        mFragmentRange = {};

        size_t authorityOffset = 0;

        // Scheme before ://
        size_t schemeEnd = remaining.find("://");
        if (schemeEnd != std::string::npos)
        {
            mSchemeRange.offset = 0;
            mSchemeRange.size = schemeEnd;
            remaining.remove_prefix(schemeEnd + 3);
            authorityOffset = schemeEnd + 3;
        }

        size_t fragmentStart = remaining.find('#');
        size_t queryStart = remaining.find('?');

        // Fragment after # and before ?
        if (fragmentStart != std::string::npos)
        {
            size_t fragmentEnd = remaining.size();
            if (queryStart != std::string::npos && queryStart > fragmentStart)
                fragmentEnd = queryStart;

            mFragmentRange.offset = authorityOffset + fragmentStart + 1;
            mFragmentRange.size = fragmentEnd - fragmentStart - 1;
        }

        // Query after ? and before #
        if (queryStart != std::string::npos)
        {
            size_t queryEnd = remaining.size();
            if (fragmentStart != std::string::npos && fragmentStart > queryStart)
                queryEnd = fragmentStart;

            mQueryRange.offset = authorityOffset + queryStart + 1;
            mQueryRange.size = queryEnd - queryStart - 1;
        }

        size_t size = remaining.size();
        size = std::min(size, queryStart);
        size = std::min(size, fragmentStart);
        remaining = remaining.substr(0, size);

        // Authority and Path
        size_t pathStart = remaining.find('/');
        if (pathStart != std::string::npos)
        {
            mAuthorityRange.offset = authorityOffset;
            mAuthorityRange.size = pathStart;
            remaining.remove_prefix(pathStart);
            mPathRange.offset = authorityOffset + mAuthorityRange.size + 1;
            mPathRange.size = remaining.size() - 1;
        }
        else
        {
            mAuthorityRange.offset = authorityOffset;
            mAuthorityRange.size = remaining.size();
        }
    }

    std::string mString;
    Range mSchemeRange;
    Range mAuthorityRange;
    Range mPathRange;
    Range mQueryRange;
    Range mFragmentRange;
};

} // namespace LD