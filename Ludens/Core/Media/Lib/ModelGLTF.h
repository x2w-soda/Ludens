#pragma once

namespace LD
{

class Path;
class Model;

void LoadModelGLTFAscii(const Path& path, Model& model);
void LoadModelGLTFBinary(const Path& path, Model& model);

} // namespace LD