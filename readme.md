# utfz - a tiny C++ library for parsing and encoding utf-8

The goal of this library is to provide a tiny set of utilities that make it easy
to iterate over the code points of a UTF8 string, as well as build up a UTF8 string
from 32-bit integer code points.

* Two source files (utfz.cpp and utfz.h)
* Does not throw exceptions
* 100% line coverage in tests
* All iteration methods support both null terminated strings, and explicit length strings
* The U+0000 code point is treated as invalid, and replaced with the standard replacement character U+FFFD
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

## Why not support the U+0000 code point?
In UTF-8, the U+0000 code point is represented by a single zero byte. In C, this
is a null terminator. Firstly, it is impossible to decode this code point when
your input data is null terminated, because by definition it's not a code point,
it's the terminator for the string. However, you could argue that one could support
decoding the U+0000 code point when decoding a string of known length, such as
you'd find inside an std::string. This is possible, but it rides on the assumption
that you're not going to be using any null terminated string processing functions
inside your code. This is not a safe assumption for the majority of C++ programs,
so we choose simply to not support it.

Imagine the case where you receive a potentially malicious string as input. You
first run a "safety-check" function on that string, and if the safety check passes,
you then run the actual business on it. It's possible that the safety check
function takes a null terminated string as input, but the actual business function
takes a string with explicit length. In such a case, an attacker can embed a 
zero byte inside the string, and store the malicious part after the zero byte.
Now, your safety check passes, and you end up processing the malicious input.

There is an alternative to supporting the zero byte as a legal character, and that
is "Modified UTF-8", which encodes U+0000 as an overlong sequence of two bytes.
While this approach does get rid of some of the security and robustness concerns,
it's hard to believe that there aren't corner cases waiting to be exploited here
too. But still, why? Unicode is not intended to be a binary-safe transport
mechanism for an arbitrary string of bits. The code point U+0000 can only be
a special record separator, or something similar, which you can deal with manually.

### Can I still use this library to decode strings with U+0000 code points?
The answer depends on how the U+0000 code point is used. The typical use case
of the U+0000 code point is as a record separator. In this case, you could argue
that this is not really a code point, but out-of-band data. To use this library
in such a case, you need to write code that looks something like this:

```cpp
const char* input = something;
const char* end_of_input = something_end;
while (...)
{
	if (*input == 0)
	{
		// process the record ...
		input++;
	}
	else
	{
		int cp;
		if (!utfz::next(input, end_of_input, cp))
			break;
		// process the code point 'cp' ...
	}
}
```

In a nutshell, before sending the byte for decoding by utfz, check if it's a zero,
and take appropriate action if it is. This is exactly the kind of thing you'll find
inside xargs source code, when given the "-0" command line option.