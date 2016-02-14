---
weight: 0
---

# Timers

This application demonstrates how to use software timers.

## The code

### C++11

This is the recommended example.
It requires C++11 or a newer version of the C++ standard.

```cpp
{% include_relative node_cpp11.cpp %}
```

### C++03

This example shows how to work-around the lack of lambdas and `std::function<>` in C++03.

```cpp
{% include_relative node_cpp03.cpp %}
```

## Running on Linux

Both versions of the application can be built with the following CMake script:

```cmake
{% include_relative CMakeLists.txt %}
```
