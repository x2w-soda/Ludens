#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Header/Range.h>
#include <Ludens/Header/View.h>

namespace LD {

/// @brief Uniform Resource Identifier
class URI
{
public:
    URI() = default;

    explicit URI(const char* cstr)
    {
        if (cstr)
            mString = cstr;

        parse();
    }

    explicit URI(const String& str)
        : mString(str.data(), str.size())
    {
        parse();
    }

    explicit URI(View view)
        : mString(view.data, view.size)
    {
        parse();
    }

    inline void clear() { mString.clear(); }
    inline bool empty() const { return mString.empty(); }
    inline const String& string() const { return mString; }
    inline View view() const { return View(mString.data(), mString.size()); }

    inline View scheme() const { return View(mString.data() + mSchemeRange.offset, mSchemeRange.size); }
    inline View authority() const { return View(mString.data() + mAuthorityRange.offset, mAuthorityRange.size); }
    inline View path() const { return View(mString.data() + mPathRange.offset, mPathRange.size); }
    inline View stem() const { return View(mString.data() + mStemRange.offset, mStemRange.size); }
    inline View query() const { return View(mString.data() + mQueryRange.offset, mQueryRange.size); }
    inline View fragment() const { return View(mString.data() + mFragmentRange.offset, mFragmentRange.size); }

    inline std::string path_string() const { return std::string((const char*)mString.data() + mPathRange.offset, mPathRange.size); }
    inline std::string stem_string() const { return std::string((const char*)mString.data() + mStemRange.offset, mStemRange.size); }

private:
    void parse()
    {
        std::string_view remaining((const char*)mString.data(), mString.size());

        mSchemeRange = {};
        mAuthorityRange = {};
        mPathRange = {};
        mStemRange = {};
        mQueryRange = {};
        mFragmentRange = {};

        uint32_t authorityOffset = 0;

        // Scheme before ://
        size_t schemeEnd = remaining.find("://");
        if (schemeEnd != std::string::npos)
        {
            mSchemeRange.offset = 0;
            mSchemeRange.size = (uint32_t)schemeEnd;
            remaining.remove_prefix(schemeEnd + 3);
            authorityOffset = static_cast<uint32_t>(schemeEnd + 3);
        }

        size_t fragmentStart = remaining.find('#');
        size_t queryStart = remaining.find('?');

        // Fragment after # and before ?
        if (fragmentStart != std::string::npos)
        {
            size_t fragmentEnd = remaining.size();
            if (queryStart != std::string::npos && queryStart > fragmentStart)
                fragmentEnd = queryStart;

            mFragmentRange.offset = static_cast<uint32_t>(authorityOffset + fragmentStart + 1);
            mFragmentRange.size = static_cast<uint32_t>(fragmentEnd - fragmentStart - 1);
        }

        // Query after ? and before #
        if (queryStart != std::string::npos)
        {
            size_t queryEnd = remaining.size();
            if (fragmentStart != std::string::npos && fragmentStart > queryStart)
                queryEnd = fragmentStart;

            mQueryRange.offset = static_cast<uint32_t>(authorityOffset + queryStart + 1);
            mQueryRange.size = static_cast<uint32_t>(queryEnd - queryStart - 1);
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
            mAuthorityRange.size = (uint32_t)pathStart;
            remaining.remove_prefix(pathStart);
            mPathRange.offset = authorityOffset + mAuthorityRange.size + 1;
            mPathRange.size = static_cast<uint32_t>(remaining.size() - 1);
            if (mPathRange.size > 0)
                parse_stem_range(remaining);
        }
        else
        {
            mAuthorityRange.offset = authorityOffset;
            mAuthorityRange.size = (uint32_t)remaining.size();
        }
    }

    void parse_stem_range(std::string_view remaining)
    {
        size_t nameOffset = remaining.find_last_of('/');
        nameOffset = nameOffset == std::string::npos ? 0 : nameOffset + 1;
        size_t extOffset = remaining.find_first_of('.', nameOffset);
        extOffset = (extOffset == std::string::npos) ? (remaining.size()) : extOffset;

        mStemRange.offset = static_cast<uint32_t>(mPathRange.offset - 1 + nameOffset);
        mStemRange.size = static_cast<uint32_t>(extOffset - nameOffset);
    }

    String mString;
    Range32 mSchemeRange;
    Range32 mAuthorityRange;
    Range32 mPathRange;
    Range32 mStemRange;
    Range32 mQueryRange;
    Range32 mFragmentRange;
};

} // namespace LD