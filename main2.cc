#include <iostream>
#include "stfu.h"


using namespace stfu;


auto runner = test("test", [] {
	std::cout << "Hello world from test";
});

runner();
