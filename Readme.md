# STFU

STFU is Simple Testing Framework for Unit tests. The end goal of this 
library was to be able to say this:

```cpp
test("Parent",[] {
    std::cout << "Parent\n";
    
    test("Child 1",[] {
        std::cout << "Child 1\n";
    });
    
    test("Child 2",[] {
        std::cout << "Child 2\n";
    });
});
```

Outputs -

```
Parent
Child 1
Parent
Child 2
```

It only has tests with a name. Tests can contain other tests. 
The framework loops over the tests multiple times such that the leaf 
tests are executed only once and sibling tests share the same environment 
as seen in the above example. If you need global state which is usually 
a bad idea, you can use globals and pass them by reference to the required
lambdas.

The library is extremely crude because that was my need. It is not 
intended to replace google test or catch. I wanted a framework that 
I can copy and paste and get going without any hurdles.

# Installing

Just copy and paste include/stfu folder in your test folder

# Features
- [X] Simple to use and understand
- [X] C++11 compatible
- [X] As few macros and possible. Macros are really hard to debug. 
  (Only one needed so far)
- [X] No dependencies
- [X] Header only
- [ ] Multi translation unit support

# Note

The only contract of the library is that leaf nodes will be executed 
only once per cycle and sibling test cases will get the same environment.
Your only interface with this library is stfu::test function and expect macro.
All other details are inside impl namespace and subject to change

# Shout outs

Huge shout outs to

1. [Alan Mellor](https://www.linkedin.com/in/alan-mellor-15177927/) 
   for the code review
2. Phil Nash and all the contributors for creating Catch2.
