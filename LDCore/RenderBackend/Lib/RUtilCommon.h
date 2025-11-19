#pragma once

#include <string>

namespace LD {

enum RFormat;
struct RPassInfo;
struct RPassInfoData;

namespace RUtil {

uint32_t get_format_texel_size(const RFormat& format);
void save_pass_info(const RPassInfo& inInfo, RPassInfoData& outData);
void load_pass_info(const RPassInfoData& inData, RPassInfo& outInfo);
void print_binding_type(const RBindingType& inType, std::string& outType);

} // namespace RUtil
} // namespace LD