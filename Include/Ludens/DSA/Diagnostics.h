#pragma once

#include <Ludens/DSA/Vector.h>
#include <string>

namespace LD {

class Diagnostics;

class DiagnosticScope
{
public:
    DiagnosticScope() = delete;
    DiagnosticScope(Diagnostics* diag, const char* name);
    DiagnosticScope(const DiagnosticScope&) = delete;
    DiagnosticScope(DiagnosticScope&&) = delete;
    ~DiagnosticScope();

    DiagnosticScope& operator=(const DiagnosticScope&) = delete;
    DiagnosticScope& operator=(DiagnosticScope&&) = delete;

    inline std::string name() const { return mName; }

private:
    Diagnostics* mDiag = nullptr;
    const std::string mName;
};

class Diagnostics
{
    friend class DiagnosticScope;

public:
    int depth() const;

    /// @brief Mark an error at the current scope.
    void mark_error(const std::string& msg);

    /// @brief Query if there are errors, may be called during any scope.
    bool get_error(Vector<std::string>& errorScopes, std::string& errorMsg);

    /// @brief Query if there are errors, concatenates error scopes with newline char.
    bool get_error(std::string& errorMsg);

private:
    void push_scope(DiagnosticScope* scope);
    void pop_scope(DiagnosticScope* scope);

private:
    Vector<DiagnosticScope*> mScopes;
    Vector<std::string> mErrorScopes;
    std::string mErrorMsg;
};

} // namespace LD