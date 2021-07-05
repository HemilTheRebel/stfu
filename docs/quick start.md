# Quick start

This framework has only one primitive - tests. Tests can contain other tests. These tests form a tree of sorts. The
framework loops through the tree in a Depth First Search manner executing each leaf only once. This ensures that all
child nodes get the same environment so to say to execute.

Here is a sample test case:

```c++
#define STFU_IMPL
#include
"stfu/stfu.h"

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
