#include <iostream>
#include <cassert>
#include "stfu.h"


int main() {
	using namespace stfu;
	
	auto runner = test("Parent", [] {
		int x = 1;
		assert(x == 1);

		std::cout << "Parent: \n";
		
		test("Child 1", [&] {
			assert(x == 1);
			x++;
			assert(x == 2);
			
			std::cout << "Child 1: \n";

			test("Grandchild 1", [&] {
				assert(x == 2);
				x++;
				assert(x == 3);
				std::cout << "Granchild 1: \n";
			});

			test("Grandchild 2", [&] {
				assert(x == 2);
				x++;
				assert(x == 3);
				std::cout << "Granchild 2: \n";
			});
		});

		test("Child 2", [&] {
			assert(x == 1);
			x += 2;
			assert(x == 3);

			std::cout << "Child 2:\n";

			test("Grandchild 1", [&] {
				assert(x == 3);
				x++;
				assert(x == 4);
				std::cout << "Granchild 1: \n";
			});

			test("Grandchild 2", [&] {
				assert(x == 3);
				x++;
				assert(x == 4);
				std::cout << "Granchild 2: \n";
			});
		});
	});

	runner();

	std::cout << "\n\n\n";
	
	runner();
}
