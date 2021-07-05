# Quick start

This framework has only one primitive - tests. Tests can contain other tests. These tests form a tree of sorts. The
framework loops through the tree in a Depth First Search manner executing each leaf only once. This ensures that all
child nodes get the same environment so to say to execute.

Here is a sample test case:

```c++
#define STFU_IMPL
#include "stfu/stfu.h"

int main() {
   stfu::test("parent",[] {
      std::cout << "Parent\n";
      
      stfu::test("Child 1",[]{
        std::cout << "Child 1\n";
      });
      
      stfu::test("Child 2",[]{
        std::cout << "Child 2\n";
      });
   });
}
```

Which outputs:

```
Parent
Child 1
Parent
Child 2
```

Folks familiar with Catch2 Would recognize this has the same behaviour as sections and they would be correct!

# Expect
The problem with the assertion macro is:

1. If you have this program:
```c++
int a = 0, b = 1;
assert(a == b);
```
The output is:
> Assertion `a == b' failed. 

But it doesn't tell you what they actual values of a and b are. That
is actually useful information for debugging. 

2. Assertion failure terminates the program instead of allowing other
test cases to continue
   
So, I wrote an expect macro. It has the same syntax as asset. I need 
this to be a macro because I want to capture the file and the line. 
If this was a C++ 20 library, I could have used source location which 
would be so much better because I would not need a single macro.

## Using expect
```cpp
stfu::test("expect", [] {
   int a = 0, b = 1;
   expect(a == b);
});
```
The output I get is:
```cpp
Assertion Failed. 
Expression: a == b
Actual: 0 != 1
```
I actually get the values of a and b and the expression that failed!
Yay!

## Special form 
Expect also can take in only one value in which case it used operator!
to report failure. For example, the following never fails 
```cpp
expect(1);
expect(true);
```

## Expect throws
Sometimes we want to check if a certain function throws a certain
exception. Like say a function should throw an exception on invalid input.
You can manually test that using this:
```cpp
try {
    // code that throws the exception
    expect(false);
} catch (exception_type& e) {
    
} catch (...) {
    expect(false);
}
```

But this happens frequent enough that I decided to provide a wrapper.
Here is an example: 
```cpp
stfu::test("vector::at throws std::out_of_range when 0th element is accessed on an empty vector", []{
    std::vector<int> a;
    expectThrows(std::out_of_range, [&] {
        expect(a.at(0) == 10);
    });
});
```
Outputs nothing. But if we comment the expect line, we get:
```
vector::at throws std::out_of_range when 0th element is accessed on an empty vector failed: Assertion Failed.
Expected: std::out_of_range
Actual: no exception thrown
```

## Tests spread out into multiple cpp files
STFU supports tests in multiple cpp files. The way to do this is to
define `STFU_IMPL` in the file containing main. And then including the
stfu headers. In all other files, include the header without defining
`STFU_IMPL`. 

Let's have a look at an example.

main.cpp:
```cpp
#define STFU_IMPL
#include "stfu/stfu.h"
#include <iostream>

int main() {
   stfu::test("main.cpp",[] {
      std::cout << "I am in main.cpp\n"; 
   });
}
```

main2.cpp:
```cpp
#include "stfu/stfu.h"

static int dummy = stfu::test("main2.cpp", [] {
    std::cout << "I am in main2.cpp\n";
});
```

main3.cpp:
```cpp
#include "stfu/stfu.h"

static int dummy = stfu::test("test if printing works", [] {
    std::cout << "I am in main3.cpp\n";
});
```

These three files compiled and linked together produces the output:
```
I am in main2.cpp
I am in main3.cpp
I am in main.cpp
```

The reason main2 and main3 are printed before main.cpp is that static 
variables are guaranteed to be initialized before main is run. 
if you want to follow the same pattern in all the files, you can rewrite
main to be:
```cpp
#define STFU_IMPL
#include "stfu/stfu.h"
#include <iostream>

static int dummy = stfu::test("main.cpp",[] {
    std::cout << "I am in main.cpp\n";
});

int main() {}
```

**Note:**
1. The order in which the tests in different files are run is not 
   determined. It is not undefined behaviour to have static variables
   in different files. It is undefined behaviour however for one of 
   that static variable to depend on another. You can search "static
   initialization order fiasco" to know more
2. It is fine for the same variable to be declared in mutliple files
   because they are static. And static also means they are internal to 
   given file (translation unit to be specific)
3. You cannot depend on the order in which the tests are run in this 
   case.