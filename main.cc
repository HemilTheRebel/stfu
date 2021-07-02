#include "stfu.h"
#include <iostream>
#include <cassert>


int main() {
    using namespace stfu;

    int parent = 0, child1 = 0, child2 = 0, grandchild1 = 0, grandchild2 = 0, grandchild3 = 0, grandchild4 = 0;
    auto runner = test("Parent", [&] {
        std::cout << "Parent\n";
        parent++;

        test("Child 1", [&] {
            std::cout << "Child 1\n";
            child1++;

            test("Grandchild 1", [&] {
                std::cout << "Grand child 1\n";
                grandchild1++;
            });

            test("Grandchild 2", [&] {
                std::cout << "Grand child 2\n";
                grandchild2++;
            });
        });

        test("Child 2", [&] {
            std::cout << "Child 2\n";
            child2++;

            test("Grandchild 3", [&] {
                std::cout << "Grand child 3\n";
                grandchild3++;
            });

            test("Grandchild 4", [&] {
                std::cout << "Grand child 4\n";
                grandchild4++;
            });
        });
    });

    runner();

    /// flush the output for debugging
    std::cout << std::endl;
    assert(grandchild1 == 1);
    assert(grandchild2 == 1);
    assert(grandchild3 == 1);
    assert(grandchild4 == 1);

    assert(child1 == 2);
    assert(child2 == 2);

    assert(parent == 4);

    runner = test("test runner can be reassigned", [] {
        test("1 == 1", [] {
            assert(1 == 1);
        });
    });

    runner();

    runner = test("expect tests", [] {
        test("test expect failure should throw an assertion exception", [] {
            try {
                expect(false == true);
                assert(false);
            } catch (impl::AssertionFailed& af) {
                std::cout << af.what();
            }
        });

        test("test expect(false) works",[]{
            try {
                expect(false);
                assert(false);
            } catch (impl::AssertionFailed& af) {
                std::cout << af.what();
            }
        });

        test("test expect(true) works",[]{
            try {
                expect(true);
            } catch (...) {
                assert(false);
            }
        });
    });

    runner();
}
