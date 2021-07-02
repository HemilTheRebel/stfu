/**
 * STFU is Simple Testing Framework for Unit tests.
 * The was the end goal of this library was to be able to say this:
 *
 * ```cpp
 * auto runner = test("Parent", [] {
 *    std::cout << "Parent\n";
 *
 *    test("Child 1", [] {
 *        std::cout << "Child 1\n";    
 *    });
 *
 *    test("Child 2", [] {
 *        std::cout << "Child 2\n";    
 *    });
 * });
 *
 * runner();
 * ```
 *
 * Outputs -
 * ```
 * Parent
 * Child 1
 * Parent
 * Child 2
 * ```
 *
 *
 * It only has tests with a name. Tests can contain other tests.
 * The framework loops over the tests multiple times such that the
 * leaf tests are only executed once and sibling tests share the same
 * environment as seen in the above example. If you need global state
 * which is usually a bad idea, you can use globals and pass them by 
 * reference to the required lambdas.

 * The library is extremely crude because that was my need.
 * This library will never exceed 500 lines of code. It is not intended
 * to replace google test or catch. I wanted a framework that I can copy
 * paste and get going without any hurdles.
 */


#include <string>
#include <functional>
#include <utility>
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>
#include <iostream>

#include "expect.h"

/**
 * Contains all the implementation details. Skip to definition of test
 * to find the public contract of this library with the outside world
 */
namespace stfu {
    namespace impl {


        /**
         * Represents the state of a test case
         */
        class test_case {


            /**
             * func - The function to execute when running the test case.
             * Corresponds to the lambda that we get in test function
             */
            const std::function<void()> func;


            /**
             * The children of this test case. Starts out as empty.
             * It is important to remember that STFU sees a n-ary tree of tests
             * So each node must know its children.
             *
             * Why is this vector of shared_ptr of test case instead of
             * vector of test case? Honestly, I dont know.
             *
             * I was getting a use after free bug. Address sanitizer said
             * that the object was destroyed in add_child in the
             * push_back and I was referring to it increment_children_executed.
             * Making it shared_ptr fixed that issue.
             */
            std::vector<std::shared_ptr<test_case>> children;


            /**
             * Index of next child to execute. But the name was already big
             * enough that I left out index.
             *
             * Every time a leaf node is executed, this is incremented to
             * execute the next child if any
             */
            size_t next_child_to_execute = 0;


            /**
             * All test cases must be run at least once. Irrespective of
             * everything else. At most places, I am checking
             * if (next_child_to_execute < children.size())
             * but this condition does'lhs hold for the first run.
             *
             * So, we need a boolean to check if this is the first run
             */
            bool first_execution = true;


            /**
             * Pointer to the parent. Will be null if test case is the
             * root test case. Otherwise will point to parent
             */
            test_case *parent;


            /**
             * Increments next_child_to_execute. If this test should no more
             * run and parent is not null, recursively increment parent's
             * next_child_to_execute.
             *
             * This informs the parent to execute the
             * next test on the next run
             */
            void increment_children_executed() {
                next_child_to_execute++;

                /// If the current test should no more run and parent is
                /// not null, notify the parent to update its state
                if (!should_run() && parent) {
                    parent->increment_children_executed();
                }
            }

        public:


            /**
             * Name of the test. Used to uniquely identify a test case
             * among its parents. If a test case with same name is added
             * twice, the second one will be discarded.
             *
             * It can also be used to display debugging information should
             * you choose to do so.
             */
            const std::string name;


            /**
             * Just assigns the values given in parameters
             */
            test_case(std::string test_name, std::function<void()> test_func, test_case *test_parent) noexcept
                    : func(std::move(test_func)), parent(test_parent), name(std::move(test_name)) {}


            /**
             * Adds a child if a child with same name doesn'lhs already exist
             * It will run the child if children is empty or if in this
             * particular cycle, this test case should be executed.
             */
            void add_child(std::shared_ptr<test_case> child) {
                auto it = std::find_if(
                        children.cbegin(),
                        children.cend(),
                        [&](const std::shared_ptr<test_case> &c) {
                            return c->name == child->name;
                        }
                );

                auto index = size_t(it - children.cbegin());
                bool child_exists = it != children.cend();

                if (!child_exists) {
                    children.push_back(child);
                    index = children.size() - 1;
                }


                if (index == next_child_to_execute) {
                    if (children[next_child_to_execute]->should_run()) {
                        try {
                            children[next_child_to_execute]->run();
                        } catch (std::exception &exception) {
                            std::cout << "Exception caught\n";
//                            failed_tests.push_back(root);
                        } catch (...) {
                            std::cout << "Exception caught\n";
//                            failed_tests.push_back(root);
                        }

                    }
                }
            }


            /**
             * Specifies whether a test case should run. Test case is only
             * run when should_run returns true.
             *
             * A test should run when either -
             * 1. Its the first execution, or
             * 2. There are children left to execute
             */
            bool should_run() {
                return first_execution
                       || next_child_to_execute < children.size();
            }


            /**
             * Runs the test case.
             *
             * This function needs to refer to impl::current_test which
             * is a test_case*. But test_case is not fully defined yet.
             * So we cannot have a pointer to test_case inside test_case
             * unless its fully defined.
             *
             * So I have declared the function over here. It will be defined
             * after impl::current_test
             */
            void run();


            /**
             * If this is not a leaf node and not all children are executed,
             * notifies the next_child_to_execute that a cycle has been
             * completed.
             *
             * If this is a leaf node and parent is not null, then notify
             * the parent to increment its next_child_to_execute.
             */
            void cycle_complete() {
                first_execution = false;

                if (next_child_to_execute < children.size()) {
                    children[next_child_to_execute]->cycle_complete();
                    return;
                }

                if (parent) {
                    parent->increment_children_executed();
                }
            }
        };


        /**
         * To execute tests, we need the root.
         *
         * It is a shared_ptr because error reporting needs a reference to
         * the test cases failed. Copying the test case for every failed
         * test might not be feasible.
         *
         * This is a top level declaration because we need the ability to reset
         * after each root level test case has been executed
         */
        std::shared_ptr<test_case> root;


        /**
         * Pointer to current test. It is important because we want to add
         * child tests at the correct level of hierarchy.
         */
        test_case *current_test;


        /**
         * Runs the test
         */
        inline void test_case::run() {
            /// Update impl::current_test because we are running now.
            /// So all nested tests are our children.
            impl::current_test = this;
            func();
            /// We have completed running. So all the children left are
            /// our parent's children
            impl::current_test = parent;
            /// we only need to set this on the first iteration of run.
            /// But adding an extra if condition did not make sense as
            /// setting false to false achieves the same effect
            first_execution = false;
        }
    } /// namespace impl
} /// namespace stfu


/**
 * Contains all the public functions.
 */
namespace stfu {
    /**
     * Techically not needed. Used to make the declaration of test fit within
     * 80 chars
     */
    using Runner = std::function<void()>;


    /**
     * This is the only thing public.
     *
     * If root is null, create a root with given name and function and return
     * a Runner which upon calling will execute the root test case.
     *
     * Else add the test_case as a child to the currently executing test
     * case
     */
    inline Runner test(const std::string &name, const std::function<void()> &func) {
        using namespace stfu::impl;

        if (!root) {
            return [=] {
                std::vector<std::shared_ptr<test_case>> failed_tests;

                /// std::make_unique not used for C++11 compatibility
                /// test_case constructor will never throw so its not a
                /// big deal
                auto *root_test = new test_case(name, func, current_test);
                root = std::shared_ptr<test_case>(root_test);

                /// We might need multiple iterations of root to execute
                /// all test cases as we are only executing 1 leaf at a time
                while (root->should_run()) {
                    try {
                        root->run();
                    } catch (std::exception &exception) {
                        std::cout << "Exception caught\n";
                        failed_tests.push_back(root);
                    } catch (...) {
                        std::cout << "Exception caught\n";
                        failed_tests.push_back(root);
                    }

                    root->cycle_complete();
                }

                /// After running all the test cases, we are resetting the nodes.
                /// This allows the runner to be called multiple times.
                /// I dont know why I added this functionality. It is probably
                /// useful for fuzzing but there you go
                root.reset();
            };
        }

        /// Ensure current test is not null. There is no case in which
        /// it should be null
        assert(current_test != nullptr);

        current_test->add_child(std::make_shared<test_case>(name, func, current_test));
        return [] {};
    }
}


