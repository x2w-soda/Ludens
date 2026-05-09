#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/DSA/Vector.h>

namespace LD {

class Diagnostics;

class DiagnosticScope
{
public:
    DiagnosticScope() = delete;
    DiagnosticScope(Diagnostics& diag, const char* name);
    DiagnosticScope(Diagnostics& diag, const String& name);
    DiagnosticScope(const DiagnosticScope&) = delete;
    DiagnosticScope(DiagnosticScope&&) = delete;
    ~DiagnosticScope();

    DiagnosticScope& operator=(const DiagnosticScope&) = delete;
    DiagnosticScope& operator=(DiagnosticScope&&) = delete;

    inline String name() const { return mName; }

private:
    Diagnostics* mDiag = nullptr;
    const String mName;
};

class Diagnostics
{
    friend class DiagnosticScope;

public:
    int depth() const;

    /// @brief Mark an error at the current scope.
    void mark_error(View msg);

    /// @brief Query if there are errors, may be called during any scope.
    bool get_error(Vector<String>& errorScopes, String& errorMsg);

    /// @brief Query if there are errors, concatenates error scopes with newline char.
    bool get_error(String& errorMsg);

private:
    void push_scope(DiagnosticScope* scope);
    void pop_scope(DiagnosticScope* scope);

private:
    Vector<DiagnosticScope*> mScopes;
    Vector<String> mErrorScopes;
    String mErrorMsg;
};

} // namespace LD