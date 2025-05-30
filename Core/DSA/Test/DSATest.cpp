#include "DSATest.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "HeapStorageTest.h"
#include "VectorTest.h"
#include <Extra/doctest/doctest.h>

int Foo::sCtor = 0;
int Foo::sDtor = 0;
int Foo::sCopyCtor = 0;
int Foo::sCopyAssign = 0;
int Foo::sMoveCtor = 0;
int Foo::sMoveAssign = 0;