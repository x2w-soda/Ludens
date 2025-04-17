#pragma once

namespace LD {

template <typename TObject>
class Handle
{
public:
    Handle() : mObj(nullptr) {}
    Handle(TObject* obj) : mObj(obj) {}

    operator bool() const { return mObj != nullptr; }
    operator TObject*() { return mObj; }
    operator const TObject*() const { return mObj; }

protected:
    TObject* mObj;
};

} // namespace LD