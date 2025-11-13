#pragma once

namespace LD {

enum RFormat;
struct RPassInfo;
struct RPassInfoData;

namespace RUtil {

uint32_t get_format_texel_size(const RFormat& format);
void save_pass_info(const RPassInfo& inInfo, RPassInfoData& outData);
void load_pass_info(const RPassInfoData& inData, RPassInfo& outInfo);

} // namespace RUtil
} // namespace LD