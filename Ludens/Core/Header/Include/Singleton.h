#pragma once

namespace LD
{

template <typename T>
class Singleton
{
public:

    /// get singleton instance
    static T& GetSingleton()
    {
        if (sInstance == nullptr)
            sInstance = new T();

        return *sInstance;
    }

    /// explict deleteion of the singleton instance, next call
    /// to GetSingleton() will create a new instance
    static void DeleteSingleton()
    {
        if (sInstance)
            delete sInstance;

        sInstance = nullptr;
    }

private:
    static T* sInstance;
};

template <typename T>
T* Singleton<T>::sInstance;

} // namespace LD