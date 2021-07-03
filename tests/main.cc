#include <iostream>
#include <cassert>
#include <stfu/stfu.h>


int main() {
    int parent = 0, child1 = 0, child2 = 0, grandchild1 = 0, grandchild2 = 0, grandchild3 = 0, grandchild4 = 0;
    stfu::test("Parent", [&] {
        parent++;

        stfu::test("Child 1", [&] {
            child1++;

            stfu::test("Grandchild 1", [&] {
                grandchild1++;
            });

            stfu::test("Grandchild 2", [&] {
                grandchild2++;
            });
        });

        stfu::test("Child 2", [&] {
            child2++;

            stfu::test("Grandchild 3", [&] {
                grandchild3++;
            });

            stfu::test("Grandchild 4", [&] {
                grandchild4++;
            });
        });
    });

    /// flush the output for debugging
    std::cout << std::endl;
    assert(grandchild1 == 1);
    assert(grandchild2 == 1);
    assert(grandchild3 == 1);
    assert(grandchild4 == 1);

    assert(child1 == 2);
    assert(child2 == 2);

    assert(parent == 4);

    stfu::test("more tests can be executed", [] {
        stfu::test("1 == 1", [] {
            assert(1 == 1);
        });
    });

    /// implicitly also tests expect works with booleans
    stfu::test("expect tests", [] {
        stfu::test("check expect failure should throw an assertion exception", [] {
            try {
                expect(false == true);
                assert(false);
            } catch (stfu::impl::AssertionFailed& af) {
                std::cout << af.what();
            }
        });

        stfu::test("check expect failure does not crash the program", [] {
            std::cout << "temp" << std::endl;
            try {
                expect(false);
            } catch (std::exception& e) {
                std::cout << e.what();
            }

        });

        stfu::test("check expect(false) works",[]{
            try {
                expect(false);
                assert(false);
            } catch (stfu::impl::AssertionFailed& af) {
                std::cout << af.what();
            }
        });

        stfu::test("check expect(true) works",[]{
            try {
                expect(true);
            } catch (...) {
                assert(false);
            }
        });
    });
}
