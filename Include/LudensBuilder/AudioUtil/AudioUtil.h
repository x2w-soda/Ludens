#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/DSP/DSP.h>
#include <cstdint>
#include <filesystem>

namespace LD {

struct AudioUtil : Handle<struct AudioUtilObj>
{
	static AudioUtil create();
	static void destroy(AudioUtil util);

	/// @brief one-shot function to resample audio file, converting sample rates and sample format.
	/// @param srcFile path to source audio file, currently only .wav is supported
	/// @param dstFile path to write resampled audio file, currently only .wav is supported
	/// @param sampleRate new sample rate
	/// @param format new sample format
	/// @return true on success
	bool resample(const std::filesystem::path& srcFile, const std::filesystem::path& dstFile, uint32_t sampleRate, SampleFormat format);
};

} // namespace LD