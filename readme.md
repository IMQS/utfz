# utfz - a tiny C++ library for parsing and encoding utf-8

* Two source files (utfz.cpp and utfz.h)
* Does not throw exceptions - uses error codes.
* 100% line coverage in tests

Example of printing code points to the console:
```cpp
const char* a_utf8_string;
for (int cp : utfz::cp(a_utf8_string))
    printf("%d ", cp);
```

Example of `tolower` using `std::string`:
```cpp
std::string input;
std::string low;
for (int cp : utfz::cp(input))
    utfz::encode(low, ::tolower(cp));
```
