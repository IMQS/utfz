// -----------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.
// -----------------------------------------------------------------------
#pragma once

#include <string>

/* A tiny UTF8 iterator library for C++.

The goal of this library is to provide a tiny set of utilities that make it easy
to iterate over the code points of a UTF8 string, as well as build up a UTF8 string
from 32-bit integer code points.

When an error is detected, the library returns utfz::replace (U+FFFD), and attempts
to restart parsing at the next legal code point.

See more in the readme.

*/
namespace utfz {

enum code
{
	invalid = -1,     // returned by seq_len when not a valid leading byte
	replace = 0xfffd, // replacement character returned when decoding fails
};

enum limits
{
	max1 = 0x7f,     // maximum code point that can be represented by one byte
	max2 = 0x7ff,    // maximum code point that can be represented by two bytes
	max3 = 0xffff,   // maximum code point that can be represented by three bytes
	max4 = 0x10ffff, // maximum code point that can be represented by four bytes
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
// a code point is truncated by a null terminator. If the returned code
// point is 0, then you have reached the end of the string.
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
// Returns the encoded size (1..4), or 0 for an invalid code point.
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

		bool  operator==(const iter& b) const { return S == b.S; }
		bool  operator!=(const iter& b) const { return S != b.S; }
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
