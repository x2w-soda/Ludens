#pragma once

#include <Ludens/Lexer/Token.h>
#include <Ludens/Lexer/Unicode.h>
#include <Ludens/System/Allocator.h>
#include <cctype>
#include <cstring>

namespace LD {

/// @brief Tokens that can be matched against an immutable string.
///        Language keywords and punctuators may use match rules.
template <typename ETokenType>
struct LexerMatchRule
{
    ETokenType type;
    const char* match;
    size_t matchLen;
};

template <typename ETokenType>
struct LexerInfo
{
    ETokenType endOfFileToken;
    ETokenType singleLineCommentToken;
    const char* singleLineComment;
    const LexerMatchRule<ETokenType>* matchRules;
    size_t matchRuleCount;
    MemoryUsage memoryUsage;
};

/// @brief Generic lexer designed to perform lexical analysis
/// 	   on a wide range of languages. Currently only supports UTF-8.
/// @tparam ETokenType Token enum type defined by the client language.
template <typename ETokenType>
class Lexer
{
public:
    /// @brief In-place startup.
    void startup(const LexerInfo<ETokenType>& info)
    {
        LinearAllocatorInfo laI{};
        laI.capacity = sizeof(Token<ETokenType>) * 256;
        laI.usage = info.memoryUsage;
        laI.isMultiPage = true;
        mTokenLA = LinearAllocator::create(laI);
        mEndOfFileToken = info.endOfFileToken;
        mSingleLineCommentToken = info.singleLineCommentToken;
        mSingleLineComment = std::string_view(info.singleLineComment);
        mMatchRules = info.matchRules;
        mMatchRuleCount = info.matchRuleCount;

        // TODO: ident1, ident2
    }

    /// @brief In-place cleanup.
    void cleanup()
    {
        LinearAllocator::destroy(mTokenLA);
    }

    /// @brief Process UTF-8 bytes into token stream.
    /// @param utf8 A valid UTF-8 byte stream.
    /// @return Token linked list on success.
    /// @warning Currently does not handle errors and is fragile.
    Token<ETokenType>* process(const uint8_t* utf8, size_t len)
    {
        Token<ETokenType> dummy = {.next = nullptr};
        Token<ETokenType>* tok = &dummy;
        ETokenType type;

        int pos = 0;

        while (pos < len)
        {
            skip_spaces(utf8, len, pos);

            const uint8_t* buf = utf8 + pos;
            int buflen = (int)len - pos;
            int advance;

            if (buflen == 0)
                break;

            // single line comment
            int matchLen = (int)mSingleLineComment.size();
            if (buflen >= matchLen && !memcmp(buf, mSingleLineComment.data(), matchLen))
            {
                pos += matchLen;
                buf = utf8 + pos;
                buflen = len - pos;
                advance = utf8_decode_line(buf, (size_t)buflen);
                std::string_view span((const char*)buf, (const char*)buf + advance);
                tok = tok->next = alloc_token(mSingleLineCommentToken, span);
                continue;
            }

            // match rules
            advance = match_rules(buf, buflen, type);
            if (advance)
            {
                std::string_view span((const char*)buf, (const char*)buf + advance);
                tok = tok->next = alloc_token(type, span);
                pos += advance;
                continue;
            }

            // TODO: unreachable.
        }

        tok = tok->next = alloc_token(mEndOfFileToken, {});

        return dummy.next;
    }

private:
    int match_rules(const uint8_t* utf8, int len, ETokenType& outType)
    {
        int pos = 0;

        for (size_t i = 0; i < mMatchRuleCount; i++)
        {
            auto& rule = mMatchRules[i];
            if (rule.matchLen >= len)
                continue;

            if (!memcmp(utf8, rule.match, rule.matchLen))
            {
                outType = rule.type;
                return rule.matchLen;
            }
        }

        return 0;
    }

    void skip_spaces(const uint8_t* utf8, int len, int& pos)
    {
        pos += utf8_decode_whitespace(utf8 + pos, (size_t)len);
    }

    Token<ETokenType>* alloc_token(ETokenType type, const std::string_view& span)
    {
        auto* tok = (Token<ETokenType>*)mTokenLA.allocate(sizeof(Token<ETokenType>));
        tok->next = nullptr;
        tok->span = span;
        tok->type = type;

        return tok;
    }

private:
    LinearAllocator mTokenLA;
    ETokenType mEndOfFileToken;
    ETokenType mSingleLineCommentToken;
    std::string_view mSingleLineComment;
    const LexerMatchRule<ETokenType>* mMatchRules;
    size_t mMatchRuleCount;
};

} // namespace LD
