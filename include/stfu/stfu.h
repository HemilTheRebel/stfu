#ifndef STFU_STFU_H
#define STFU_STFU_H

#include <string>
#include <functional>
#include <utility>
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>
#include <iostream>

#include "expect.h"

namespace stfu {
    /// int is a dummy return value. You are free to ignore this for
    /// single file test cases. But we need to assign some value to
    /// a static variable to be able to call a function in another
    /// translation unit automatically on executing
    int test(const std::string &name, const std::function<void()> &func);
}

/// Implementation details. Subject to change
#ifdef STFU_IMPL
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

                /// We assume test cases remain same. That's the contract.
                /// If in the first execution, we find a child exists, its
                /// a human error. Two test cases with same name are present.
                /// Notify the user and bring down the program
                if (child_exists && first_execution) {
                    throw std::runtime_error("Two tests with same name detected");
                }

                if (index == next_child_to_execute) {
                    if (children[next_child_to_execute]->should_run()) {
                        try {
                            children[next_child_to_execute]->run();
                        } catch (std::exception &e) {
                            std::cout << name << " failed: " << e.what() << '\n';
//                            failed_tests.push_back(root);
                        } catch (...) {
                            std::cout << "Unknown exception caught\n";
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
        void test_case::run() {
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

        void run_tests(const std::string &name, const std::function<void()> &func) {
            std::vector<std::shared_ptr<impl::test_case>> failed_tests;

            /// std::make_unique not used for C++11 compatibility
            /// test_case constructor will never throw so its not a
            /// big deal
            auto *root_test = new impl::test_case(name, func, impl::current_test);
            impl::root = std::shared_ptr<impl::test_case>(root_test);

            /// We might need multiple iterations of root to execute
            /// all test cases as we are only executing 1 leaf at a time
            while (impl::root->should_run()) {
                try {
                    impl::root->run();
                } catch (std::exception &exception) {
                    std::cout << name << " failed: " << exception.what() << '\n';
                    failed_tests.push_back(impl::root);
                } catch (...) {
                    std::cout << "Unknown exception caught\n";
                    failed_tests.push_back(impl::root);
                }

                impl::root->cycle_complete();
            }

            /// After running all the test cases, we are resetting the nodes.
            /// This allows the runner to be called multiple times.
            /// I dont know why I added this functionality. It is probably
            /// useful for fuzzing but there you go
            impl::root.reset();
            impl::current_test = nullptr;
        }
    } /// namespace impl

    int test(const std::string &name, const std::function<void()> &func) {
        using namespace stfu::impl;

        if (!root) {
            run_tests(name, func);
            return 0;
        }

        /// Ensure current test is not null. There is no case in which
        /// it should be null
        assert(current_test != nullptr);
        current_test->add_child(std::make_shared<test_case>(name, func, current_test));
        return 0;
    }
} /// namespace stfu
#endif /// end if for STFU_IMPL

#endif //STFU_STFU_H
