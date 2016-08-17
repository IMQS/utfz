#pragma once

#include <string>

/* A tiny UTF8 iterator library for C++.

The goal of this library is to provide a tiny set of utilities that make it easy
to iterate over the code points of a UTF8 string, as well as build up a UTF8 string.

When decoding utf-8, this library provides two paths:
1. If you specify the string length, then NUL characters are allowed, producing the U+0000 code point.
2. If you do not specify the string length, then NUL characters are treated as a null terminator.

In other words, option (2) is restricted to what is called "Modified UTF-8"
(see https://en.wikipedia.org/wiki/UTF-8). Basically, Modified UTF-8 forces the NUL code point
to be encoded as the two-byte sequence 0xC080.

Decoding functions that accept only Modified UTF-8 are suffixed with _modified. These functions
also do not take a length parameter, but assume instead that the string is null terminated.

The encoding functions always produce Modified UTF-8.
*/
namespace utfz {

enum code
{
	invalid = -1,
};

// Returns the sequence length (1,2,3,4) or 'invalid' if not a valid leading byte.
// Returns 1 for the NUL character (i.e. c = 0)
int seq_len(char c);

// Returns the sequence length (1,2,3,4) or 'invalid' if not a valid leading byte of a "Modified UTF-8".
// Returns 0 if c == 0, assuming that 'c' is the null terminator of a string.
int seq_len_modified(char c);

// Decode a code point. Returns the code point, or 'invalid'
// If seq_len is not null, and the return value is not 'invalid', then seq_len holds the sequence length.
int decode(const char* s, const char* end);
int decode(const char* s, const char* end, int& seq_len);

// A variant of decode where you do not know the length of the string.
// This assumes the string is null terminated, and returns 'invalid' if
// a code point is truncated by a null terminator.
int decode(const char* s);
int decode(const char* s, int& seq_len);

// Returns the code point at 's', and increments 's' so that it points to the
// start of the next code point.
// If 's' is not a valid code point (for whatever reason, including buffer
// overflow), then 'invalid' is returned.
int next(const char*& s, const char* end);

// A variant of next where you do not know the length of the string
// This assumes the string is null terminated, and returns 'invalid' if
// a code point is truncated by a null terminator.
int next(const char*& s);

// Encode the code point 'cp', into the buffer 'buf', using "Modified UTF-8".
// Returns the encoded size (1..4), or 0 for an invalid code point.
int encode(char* buf, int cp);

// Encode the code point 'cp', adding it to the string 's' using "Modified UTF-8" encoding.
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
// If an error is detected, then the iteration aborts immediately.
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
		template<bool is_check>
		void increment();

		void check_first();
		bool known_end() const;
	};

	iter begin() { return iter(Str, End); }
	iter end() { return iter(End, End); }

	iter cbegin() const { return iter(Str, End); }
	iter cend() const { return iter(End, End); }
};
}
