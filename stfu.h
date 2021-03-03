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
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>


/**
 * Contains all the implementation details. Skip to definition of test
 * to find the public contract of this library with the outside world
 */
namespace stfu::impl {

	
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
		 * I used std::unique_ptr to express my intent of ownership.
		 * Parents own the children
		 */
		std::vector<std::unique_ptr<test_case>> children;


		/**
		 * Index of next child to execute. But the name was already big
		 * enough that I left out index.
		 *
		 * Every time a leaf node is executed, this is incremented to 
		 * execute the next child if any
		 */
		int next_child_to_execute = 0;


		/**
		 * All test cases must be run at least once. Irrespective of
		 * everything else. At most places, I am checking 
		 * if (next_child_to_execute < children.size()) 
		 * but this condition does't hold for the first run.
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
		 * Find the index of child with the same name in children.
		 * If none exists, return -1
		 */
		int index_of(test_case *child) {
			auto it = std::find_if(
				children.cbegin(),
				children.cend(),
				[&] (const std::unique_ptr<test_case> &c) {
				   return c->name == child->name;
				}
			);

			bool child_exists = it != children.cend();
			if (child_exists) {
				return it - children.cbegin();
			}

			return -1;
		}
		

		/**
		 * Notifies parent that this test case's all children have been
		 * executed. So the parent should update its next_child_to_execute.
		 *
		 * Recursively calls this function on its parents if the test case
		 * should not run and parent is not null. 
		 */
		void notify_all_children_executed() {
			next_child_to_execute++;
			if (!should_run() && parent) {
				parent->notify_all_children_executed();
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
		test_case(const std::string name, std::function<void()> func, test_case *parent)
			:name(name), func(func), parent(parent) {}


		/**
		 * Adds a child if a child with same name doesn't already exist
		 * It will run the child if children is empty or if in this 
		 * particular cycle, this test case should be executed.
		 */
		void add_child(std::unique_ptr<test_case> child) {
			int index = index_of(child.get());
			bool child_does_not_exist = index == -1;
		
			if (child_does_not_exist) {
				children.push_back(std::move(child));
				index = children.size() - 1;
			}

			if (index == next_child_to_execute) {
				if (children[next_child_to_execute]->should_run()) {
					children[next_child_to_execute]->run();				
				}
			}
		}


		/**
		 * Specifies whether a test case should run. Test case is only 
		 * run when should_run returns true
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
		 * Resets test case and all its children as if they are declared
		 * But never called yet. 
		 */
		void reset() {
			next_child_to_execute = 0;
			first_execution = true;

			for (std::unique_ptr<test_case> &child : children) {
				child->reset();
			}
		}
		

		/**
		 * If this is not a leaf node and not all children are executed,
		 * notifies the next_child_to_execute that a cycle has been 
		 * completed. 
		 *
		 * If this is a leaf node, then notify the parent that all children
		 * are executed. So as to updated parent's next_child_to_execute.
		 */
		void cycle_complete() {
			if (next_child_to_execute < children.size()) {
				children[next_child_to_execute]->cycle_complete();
				return;
			}

			if (parent) {
				parent->notify_all_children_executed();
			}
		}
	};


	/**
	 * To execute tests, we need a pointer to the root. 
	 */
	std::unique_ptr<test_case> root;


	/**
	 * Pointer to current test. It is important because we want to add
	 * child tests at the correct level of hierary. 
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
}; /// namespace stufu::impl


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
	inline Runner test(std::string name, std::function<void()> func) {
		using namespace stfu::impl;	

		auto child = std::make_unique<test_case>(name, func, current_test);
	
		if (!root) {
			root = std::move(child);
		
			return [=] {
				/// We might need multiple iterations of root to execute
				/// all test cases as we are only executing 1 leaf at a time
			
				while(root->should_run()) {				
					root->run();
					root->cycle_complete();
				}

				/// After running all the test cases, we are resetting the nodes.
				/// This allows the runner to be called multiple times.
				/// I dont know why I added this functionality. It is probably
				/// useful for fuzzing but there you go
				///
				/// Note this is not std::unique_ptr::reset.
				/// This is impl::test_case::reset
				root->reset();
			};
		}

		/// Ensure current test is not null. There is no case in which
		/// it should be null
		assert(current_test != nullptr);

		current_test->add_child(std::move(child));	
		return [] {};
	}
};


