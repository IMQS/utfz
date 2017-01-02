# utfz - a tiny C++ library for parsing and encoding utf-8

The goal of this library is to provide a tiny set of utilities that make it easy
to iterate over the code points of a UTF8 string, as well as build up a UTF8 string
from 32-bit integer code points.

* Two source files (utfz.cpp and utfz.h)
* Does not throw exceptions
* 100% line coverage in tests
* All iteration methods support both null terminated strings, and explicit length strings
* Checks for all invalid code points (overlong sequences, UTF-16 surrogate pairs, 0xFFFE, 0xFFFF)
* Returns the replacement character U+FFFD for any invalid sequences, and continues parsing on the next plausible code point

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
// 'low' now contains the lower-case representation of 'input'
```

Iterating manually, over a null terminated string:
```cpp
const char* pos = input; // input is const char*
int cp;
while (utfz::next(pos, cp))
	printf("%d ", cp);
```

Iterating manually, over a string with known length:
```cpp
const char* pos = input;       // input is const char*
const char* end = input + len;
int cp;
while (utfz::next(pos, end, cp))
	printf("%d ", cp);
```
