#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Diagnostics.h>

using namespace LD;

TEST_CASE("Diagnostics")
{
    Diagnostics diag;
    CHECK(diag.depth() == 0);
    {
        DiagnosticScope s1(&diag, "scope1");
        CHECK(diag.depth() == 1);
        {
            DiagnosticScope s2(&diag, "scope2");
            CHECK(diag.depth() == 2);
            {
                DiagnosticScope s3(&diag, "scope3");
                CHECK(diag.depth() == 3);
            }
            CHECK(diag.depth() == 2);

            diag.mark_error("feel like failing today");
        }
        CHECK(diag.depth() == 1);
    }
    CHECK(diag.depth() == 0);

    Vector<std::string> errScopes;
    std::string errMsg;
    CHECK(diag.get_error(errScopes, errMsg));
    CHECK(errScopes.size() == 2);
    CHECK(errScopes[0] == "scope1");
    CHECK(errScopes[1] == "scope2");
    CHECK(errMsg == "feel like failing today");
}

TEST_CASE("Diagnostics error at depth 0")
{
    Diagnostics diag;
    CHECK(diag.depth() == 0);
    diag.mark_error("failing at depth 0");
    Vector<std::string> errScopes;
    std::string errMsg;
    CHECK(diag.get_error(errScopes, errMsg));
    CHECK(errScopes.empty());
    CHECK(errMsg == "failing at depth 0");
    CHECK(diag.depth() == 0);
}