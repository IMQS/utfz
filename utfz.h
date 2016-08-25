#pragma once

#include <string>

/* A tiny UTF8 iterator library for C++.

The goal of this library is to provide a tiny set of utilities that make it easy
to iterate over the code points of a UTF8 string, as well as build up a UTF8 string.

One notable exception to the UTF-8 specification is that this library doesn't allow
the NUL (U+0000) code point at all. Attempting to decode or encode the NUL code point
will result in an "invalid" response.

How is this justified?

To turn the question around, why is it reasonable to use the NUL code point in a string?
Unicode is clearly an encoding designed to store text. It's true that Unicode brings
with it the ASCII history, so it includes some of the non-textual code points, commonly
referred to as "Control Codes", such as Tab and Line Feed. These are useful in the context
of storing text. However, what does one need the NUL code point for? If you want a record
separator, pick any of the other esoteric ASCII characters. For example, U+001E "Record Separator"
was designed exactly for that. Or how about U+0004 "End-of-transmission character"? What
*precisely* do you need that NUL code point for?

On the other hand, the risks posed to C/C++ software are significant.

Before deciding that you need to support NUL code points, think about your data and if possible,
test some sample data to see whether supporting NUL is actually necessary.

Examples of real-world U+0000 usage:

find . -print0 | xargs -0 -n1 ls

In this case, find and xargs work together, by agreeing that find results are separated by
a zero byte, and xargs understands that it's inputs are separated by the zero byte. This
works around the classic problems that arise when trying to correctly escape command-line
parameters. I think it can be reasonably argued that the zero byte in this case is
"out of band". In other words, 'find' is not sending a contiguous UTF-8 string that
contains U+0000 code points. Rather, it is using the zero byte to signify the end of
one string, and the start of another. So this is not an example of the U+0000 code point
in action, but rather a counter-example. If you look at the source code of various xargs
implementations, they all parse the arguments by inspecting byte after byte, and perform
the record separation manually. If they were to do any UTF-8 decoding (which they don't
actually need to do), then they would do so AFTER separating the records at zero byte
boundaries.

*/
namespace utfz {

enum code
{
	invalid = -1,
};

enum limits
{
	max1    = 0x7f,     // maximum code point that can be represented by one byte
	max2    = 0x7ff,    // maximum code point that can be represented by two bytes
	max3    = 0xffff,   // maximum code point that can be represented by three bytes
	max4    = 0x10ffff, // maximum code point that can be represented by four bytes
	replace = 0xfffd,   // replacement character returned when decoding fails
};

// Returns the sequence length (1,2,3,4) or 'invalid' if not a valid leading byte,
// which includes the case where c = 0.
int seq_len(char c);

// Move forward, returning the position of the next plausible start character
// If a zero byte is found, then the position of that byte is returned.
// If 's' is on a zero byte when restart is called, then 's' is returned.
const char* restart(const char* s);

// Move forward, returning the position of the next plausible start character
// If the end of the buffer is reached, then 'end' is returned.
const char* restart(const char* s, const char* end);

// Decode a code point. Returns the code point, or 'replace' if invalid.
// If return value is not 'replace', then seq_len holds the sequence length.
int decode(const char* s, const char* end);
int decode(const char* s, const char* end, int& seq_len);

// A variant of decode where you do not know the length of the string.
// This assumes the string is null terminated, and returns 'replace' if
// a code point is truncated by a null terminator.
int decode(const char* s);
int decode(const char* s, int& seq_len);

// Returns the code point at 's', and increments 's' so that it points to the
// start of the next plausible code point, or 'end' if the next plausible
// code point exceeds end.
// If 's' is not a valid code point (for whatever reason, including buffer
// overflow), then cp is set to 'replace'.
// Returns true if 's' was incremented.
bool next(const char*& s, const char* end, int& cp);

// A variant of next where you do not know the length of the string
// This assumes the string is null terminated.
// Returns true if 's' was incremented.
bool next(const char*& s, int& cp);

// Encode the code point 'cp', into the buffer 'buf'.
// Returns the encoded size (1..4), or 0 for an invalid code point, which includes
// the case where cp = 0.
int encode(char* buf, int cp);

// Encode the code point 'cp', adding it to the string 's'.
// If the code point is invalid, then nothing is written.
// Returns true if the code point is valid.
bool encode(std::string& s, int cp);

// Code Point iterator over a utf8 string
//
// example:
//
//   // 'str' is a const char*, or an std::string
//   for (int cp : utfz::cp(str))
//       printf("%d ", cp);          // cp is an integer code point
//
// If an error is detected, then the replace code point is emitted,
// and iteration continues on the next plausible code point.
class cp
{
public:
	const char* Str;
	const char* End;

	cp(const char* str, size_t len = -1);
	cp(const std::string& s);

	class iter
	{
	public:
		const char* S;
		const char* End; // This is null if length is unknown

		iter(const char* s, const char* end);

		bool operator==(const iter& b) const { return S == b.S; }
		bool operator!=(const iter& b) const { return S != b.S; }
		iter& operator++();
		iter& operator++(int);

		int operator*() const;

	private:
		void increment();

		bool known_end() const;
	};

	iter begin() { return iter(Str, End); }
	iter end() { return iter(End, End); }
	iter cbegin() const { return iter(Str, End); }
	iter cend() const { return iter(End, End); }
};
}
