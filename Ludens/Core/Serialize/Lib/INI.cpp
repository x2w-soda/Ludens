#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <cstdio>
#include "Core/Serialize/Include/INI.h"


namespace LD {

	void INIParser::Config(const INIParserConfig& config)
	{
		mConfig = config;
	}

	void INIParser::ParseString(const char* src, const INIParserCallback* callbacks)
	{
		if (!callbacks)
			return;

		mUserCallback = callbacks;
		mIsInSectionName = false;
		mLineNo = 0;

		const char* lineBegin = src;
		const char* lineEnd;
		while (lineBegin != nullptr)
		{
			lineEnd = strchr(lineBegin, '\n');
			ParseLine(lineBegin, lineEnd);
			if (lineEnd != nullptr)
				lineEnd++;
			lineBegin = lineEnd;
		}
	}

	void INIParser::ParseLine(const char* lineBegin, const char* lineEnd)
	{
		if (lineEnd == nullptr)
			lineEnd = strchr(lineBegin, '\n');
		if (lineEnd == nullptr)
			lineEnd = lineBegin + strlen(lineBegin);

		int lineLen = lineEnd - lineBegin;
		mLineNo++;
		mIsInSectionName = false;
		mIsInPropertyName = false;
		mIsInPropertyValue = false;

		for (int i = 0; i < lineLen; i++)
		{
			char c = lineBegin[i];
			if (isspace(c) && !mIsInSectionName)
				continue;

			if (c == '[')
			{
				mSectionNameLen = 0;
				mIsInSectionName = true;
				continue;
			}
			else if (c == ']' && mIsInSectionName)
			{
				mIsInSectionName = false;

				mSectionNameBuf[mSectionNameLen] = '\0';
				if (mUserCallback && mUserCallback->OnSection)
					mUserCallback->OnSection(mConfig.UserData, mLineNo, mSectionNameBuf);
				continue;
			}
			else if (c == '=' && mIsInPropertyName)
			{
				mIsInPropertyName = false;

				for (int* idx = &mPropertyNameLen; *idx > 0 && isspace(mPropertyNameBuf[*idx - 1]); (*idx)--) ;
				mPropertyNameBuf[mPropertyNameLen] = '\0';

				mPropertyValueLen = 0;
				mIsInPropertyValue = true;
				continue;
			}
			else if (!mIsInPropertyValue && !mIsInPropertyName)
			{
				mPropertyNameLen = 0;
				mIsInPropertyName = true;
			}


			if (mIsInSectionName)
			{
				assert(mSectionNameLen < MAX_SECTION_NAME_LEN);
				mSectionNameBuf[mSectionNameLen++] = c;
			}
			else if (mIsInPropertyName)
			{
				assert(mPropertyNameLen < MAX_PROPERTY_NAME_LEN);
				mPropertyNameBuf[mPropertyNameLen++] = c;
			}
			else if (mIsInPropertyValue)
			{
				assert(mPropertyValueLen < MAX_PROPERTY_VALUE_LEN);
				mPropertyValueBuf[mPropertyValueLen++] = c;
			}
		}

		if (mIsInPropertyValue)
		{
			for (int* idx = &mPropertyValueLen; *idx > 0 && isspace(mPropertyValueBuf[*idx - 1]); (*idx)--) ;
			mPropertyValueBuf[mPropertyValueLen] = '\0';

			if (mUserCallback && mUserCallback->OnProperty)
				mUserCallback->OnProperty(mConfig.UserData, mLineNo, mPropertyNameBuf, mPropertyValueBuf);
		}
	}

	const char* INIWriter::ViewOutput()
	{
		return mOutput.c_str();
	}

	size_t INIWriter::ViewSize()
	{
		return mOutput.size();
	}

	INIWriter& INIWriter::Write(const char* text)
	{
		mOutput.append(text);
		return *this;
	}

	INIWriter& INIWriter::WriteSection(const char* section)
	{
		mOutput.append("[");
		mOutput.append(section);
		mOutput.append("]");
		mOutput.append(mConfig.WriteSingleLine ? " " : "\n");

		return *this;
	}

	INIWriter& INIWriter::WriteProperty(const char* name, const char* value)
	{
		mOutput.append(name);
		mOutput.append("=");
		mOutput.append(value);
		mOutput.append(mConfig.WriteSingleLine ? " " : "\n");

		return *this;
	}


} // namespace LD