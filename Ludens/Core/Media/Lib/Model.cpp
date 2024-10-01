#include <iostream>
#include "Core/OS/Include/Time.h"
#include "Core/IO/Include/FileSystem.h"
#include "Core/Media/Include/Model.h"
#include "Core/Media/Lib/ModelOBJ.h"
#include "Core/Media/Lib/ModelGLTF.h"

namespace LD
{

ModelLoader::ModelLoader()
{
}

ModelLoader::~ModelLoader()
{
}

Ref<Model> ModelLoader::LoadModel(const Path& path)
{
    std::string ext = path.Extension().ToString();

    bool exists = File::Exists(path);
    LD_DEBUG_ASSERT(exists);

    if (!exists)
        return nullptr;

    Ref<Model> model = MakeRef<Model>();

    Timer timer{};
    timer.Start();

    if (ext == ".obj")
    {
        LoadModelOBJ(path, *model);
    }
    else if (ext == ".gltf")
    {
        LoadModelGLTFAscii(path, *model);
    }
    else if (ext == ".glb")
    {
        LoadModelGLTFBinary(path, *model);
    }
    else
    {
        // TODO: error handling
        LD_DEBUG_UNREACHABLE;
    }

    timer.Stop();
    double loadTime = timer.GetMilliSeconds();

    size_t vertices = 0;
    for (auto& mesh : model->Meshes)
    {
        vertices += mesh.first.Vertices.Size();
    }

    printf("ModelLoader::LoadModel [%s] %d meshes, %d vertices, %.3f ms\n", path.ToString().c_str(),
           (int)model->Meshes.Size(), (int)vertices, loadTime);

    return model;
}

LoadModelJob::LoadModelJob(const Path& path, Ref<Model>* model) : mPath(path), mModel(model), mLoadTimeMS(-1.0)
{
    Job LoadModelJob;
    LoadModelJob.Data = this;
    LoadModelJob.Main = &LoadModelJob::JobMain;
    LoadModelJob.Type = JobType::LoadModel;

    JobSystem::GetSingleton().Submit(LoadModelJob);
}

void LoadModelJob::JobMain(void* data)
{
    LoadModelJob& job = *static_cast<LoadModelJob*>(data);

    job.mHasCompleted = false;
    {
        ScopeTimer timer(&job.mLoadTimeMS);
        *job.mModel = job.mLoader.LoadModel(job.mPath);
    }
    job.mHasCompleted = true;
}

} // namespace LD