//
// Created by hemil on 04/07/21.
//

#include <stfu/stfu.h>

static int dummy = stfu::test("test if printing works", [] {
    std::cout << "Hello World from main2.cpp\n";
});