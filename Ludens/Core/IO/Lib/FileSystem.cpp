#include <iostream>
#include <fstream>
#include "Core/IO/Include/FileSystem.h"
#include "Core/OS/Include/Memory.h"
#include "Core/Header/Include/Error.h"

namespace LD {

	Path::Path(const std::string& str)
		: mPath(str)
	{
	}

	Path::Path(const std::filesystem::path& path)
		: mPath(path)
	{
	}

	Path::Path(const char* str)
		: mPath(str)
	{
	}

	File::File()
		: mSize(0)
		, mData(nullptr)
	{
	}

	File::~File()
	{
		Close();
	}

	bool File::Exists(const Path& path)
	{
		auto& std_path = static_cast<const std::filesystem::path&>(path);
		return std::filesystem::exists(std_path);
	}

	bool File::Open(const Path& path, FileMode mode)
	{
		mMode = mode;

		// TODO: currently only accepts ascii string path

		if (mMode == FileMode::Read)
		{
			std::string str = static_cast<std::filesystem::path>(path).string();
			std::ifstream stream(str.c_str(), std::ios::ate | std::ios::binary);
			LD_DEBUG_ASSERT(stream.is_open());

			mSize = stream.tellg();
			mData = (u8*)MemoryAlloc(mSize);

			stream.seekg(0);
			stream.read((char*)mData, mSize);
			stream.close();

			return true;
		}
		else if (mMode == FileMode::Write)
		{
			mWritePath = path;
			return true;
		}

		return false;
	}

	void File::Close()
	{
		mMode = FileMode::None;

		if (mData)
		{
			MemoryFree((void*)mData);
			mData = nullptr;
		}
	}

	void File::Write(const u8* data, size_t size)
	{
		LD_DEBUG_ASSERT(mMode == FileMode::Write);

		std::ofstream stream(mWritePath.ToString(), std::ios::binary);
		LD_DEBUG_ASSERT(stream.is_open());

		stream.write((const char*)data, size);
		stream.close();
	}

	void File::ReadString(std::string& string)
	{
		string.resize(mSize);
		
		if (mSize > 0)
			memcpy(string.data(), mData, mSize);
	}

	FileSystem::FileSystem()
	{
	}

	Path FileSystem::GetWorkingDirectory()
	{
		return { std::filesystem::current_path() };
	}

	bool FileSystem::CreateDirectories(const Path& path)
	{
		auto& std_path = static_cast<const std::filesystem::path&>(path);
		return std::filesystem::create_directories(std_path);
	}

} // namespace LD
