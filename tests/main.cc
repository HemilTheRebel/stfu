#define STFU_IMPL

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

    stfu::test("two tests with same name should throw runtime error", [] {
        stfu::test("abc", [] {});
        try {
            stfu::test("abc", [] {});
            expect(false);
        } catch (std::exception &e) {
            std::cout << e.what() << '\n';
        }
    });

    /// implicitly also tests expect works with booleans
    stfu::test("expect tests", [] {
        stfu::test("check expect failure should throw an assertion exception", [] {
            try {
                expect(false == true);
                assert(false);
            } catch (stfu::impl::AssertionFailed &af) {
                std::cout << af.what() << '\n';
            }
        });

        stfu::test("check expect failure does not crash the program", [] {
            std::cout << "temp" << std::endl;
            try {
                expect(false);
            } catch (std::exception &e) {
                std::cout << e.what() << '\n';
            }
        });

        stfu::test("check expect(false) works", [] {
            try {
                expect(false);
                assert(false);
            } catch (stfu::impl::AssertionFailed &af) {
                std::cout << af.what() << '\n';
            }
        });

        stfu::test("check expect(true) works", [] {
            try {
                expect(true);
            } catch (...) {
                assert(false);
            }
        });

        stfu::test("expect 1 succeeds", [] {
            expect(1);
        });
    });

    stfu::test("just trying to see the error message when test fails", [] {
        expect(false);
    });

    stfu::test("expectThrows tests", [] {
        stfu::test("expectThrows does nothing when exception of the given type is thrown", [] {
            expectThrows(int, [] { throw 0; });
            std::cout << "expectThrows caught the exception\n";
        });

        stfu::test("expectThrows throws AssertionFailure when the callable does not throw the object of the right type", [] {
            try {
                expectThrows(std::string, [] { throw 0; });
                expect(false);
            } catch (stfu::impl::AssertionFailed& e) {
                std::cout << e.what() << '\n';
                expect(e.expected == "std::string");
                expect(e.actual == "unknown");
            }
        });

        stfu::test("expectThrows throws AssertionFailed when the callable does not throw", [] {
            try {
                std::vector<int> a;
                expectThrows(std::out_of_range, [&] {});
                expect(false);
            } catch (stfu::impl::AssertionFailed& e) {
                std::cout << e.what() << '\n';
            } catch (...) {
                expect(false);
            }
        });
    });
}
