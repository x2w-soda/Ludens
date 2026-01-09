#include <Ludens/DSA/Diagnostics.h>
#include <Ludens/Header/Assert.h>

namespace LD {

DiagnosticScope::DiagnosticScope(Diagnostics& diag, const char* name)
    : mDiag(&diag), mName(name)
{
    mDiag->push_scope(this);
}

DiagnosticScope::DiagnosticScope(Diagnostics& diag, const std::string& name)
    : mDiag(&diag), mName(name)
{
    mDiag->push_scope(this);
}

DiagnosticScope::~DiagnosticScope()
{
    mDiag->pop_scope(this);
}

int Diagnostics::depth() const
{
    return (int)mScopes.size();
}

void Diagnostics::mark_error(const std::string& msg)
{
    LD_ASSERT(mErrorMsg.empty()); // already in error recovery

    mErrorMsg = msg;
    mErrorScopes.resize(mScopes.size());
    for (size_t i = 0; i < mScopes.size(); i++)
        mErrorScopes[i] = mScopes[i]->name();
}

bool Diagnostics::get_error(Vector<std::string>& errorScopes, std::string& errorMsg)
{
    if (mErrorMsg.empty())
        return false;

    errorScopes = mErrorScopes;
    errorMsg = mErrorMsg;
    return true;
}

bool Diagnostics::get_error(std::string& errorMsg)
{
    errorMsg.clear();

    std::string msg;
    Vector<std::string> scopes;
    if (!get_error(scopes, msg))
        return false;

    for (auto str : scopes)
    {
        errorMsg += str;
        errorMsg.push_back('\n');
    }
    errorMsg += msg;

    return true;
}

void Diagnostics::push_scope(DiagnosticScope* scope)
{
    mScopes.push_back(scope);
}

void Diagnostics::pop_scope(DiagnosticScope* scope)
{
    LD_ASSERT(!mScopes.empty() && mScopes.back() == scope);

    mScopes.pop_back();
}

} // namespace LD