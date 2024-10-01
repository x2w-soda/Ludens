#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "Core/DSA/Tests/DSATests.h"
#include "Core/DSA/Tests/TestBuffer.h"
#include "Core/DSA/Tests/TestArray.h"
#include "Core/DSA/Tests/TestStack.h"
#include "Core/DSA/Tests/TestVector.h"
#include "Core/DSA/Tests/TestString.h"
#include "Core/DSA/Tests/TestStringHash.h"
#include "Core/DSA/Tests/TestOptional.h"

int Foo::CtorCounter = 0;
int Foo::DtorCounter = 0;