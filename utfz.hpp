// -----------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.
// -----------------------------------------------------------------------
#pragma once
#ifndef UTFZ_HPP_INCLUDED
#define UTFZ_HPP_INCLUDED

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

// Returns the sequence length (1,2,3,4) or 'invalid' if not a valid leading byte
inline int seq_len(char c);

// Move forward by at least one byte, returning the position of the next plausible
// start byte.
// If a zero byte is found, then the position of that byte is returned.
// If 's' is on a zero byte when restart is called, then 's' is returned.
inline const char* restart(const char* s);

// Move forward by at least one byte, returning the position of the next plausible
// start byte.
// If the end of the buffer is reached, then 'end' is returned.
inline const char* restart(const char* s, const char* end);

// Decode a code point. Returns the code point, or 'replace' if invalid.
// If return value is not 'replace', then seq_len holds the sequence length.
inline int decode(const char* s, const char* end);
inline int decode(const char* s, const char* end, int& seq_len);

// A variant of decode where you do not know the length of the string.
// This assumes the string is null terminated, and returns 'replace' if
// a code point is truncated by a null terminator. If the returned code
// point is 0, then you have reached the end of the string.
inline int decode(const char* s);
inline int decode(const char* s, int& seq_len);

// Returns the code point at 's', and increments 's' so that it points to the
// start of the next plausible code point, or 'end' if the next plausible
// code point exceeds end.
// If 's' is not a valid code point (for whatever reason, including buffer
// overflow), then cp is set to 'replace'.
// Returns true if 's' was incremented.
inline bool next(const char*& s, const char* end, int& cp);

// A variant of next where you do not know the length of the string
// This assumes the string is null terminated.
// Returns true if 's' was incremented.
inline bool next(const char*& s, int& cp);

// Encode the code point 'cp', into the buffer 'buf'.
// Returns the encoded size (1..4), or 0 for an invalid code point.
inline int encode(char* buf, int cp);

// Encode the code point 'cp', adding it to the string 's'.
// If the code point is invalid, then nothing is written.
// Returns true if the code point is valid.
inline bool encode(std::string& s, int cp);

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
} // namespace utfz

// -----------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.
// -----------------------------------------------------------------------
#include <stdint.h>
#include <string.h>

namespace utfz {

enum
{
	min_cp_2             = 0x80,    // minimum code point that is allowed to be encoded with 2 bytes
	min_cp_3             = 0x800,   // minimum code point that is allowed to be encoded with 3 bytes
	min_cp_4             = 0x10000, // minimum code point that is allowed to be encoded with 4 bytes
	utf16_surrogate_low  = 0xd800,
	utf16_surrogate_high = 0xdfff,
	invalid_fffe         = 0xfffe, // this is used for BOM detection
	invalid_ffff         = 0xffff, // don't know why this is illegal
};

// Index from the high 5 bits of the first byte in a sequence to the length of the sequence
// Imperative that -1 == invalid
const int8_t seq_len_table[32] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //  0..15 (00000..01111)
    -1, -1, -1, -1, -1, -1, -1, -1,                 // 16..23 (10000..10111) - 10... is only legal as prefixes for continuation bytes
    2, 2, 2, 2,                                     // 24..27 (11000..11011)
    3, 3,                                           // 28..29 (11100..11101)
    4,                                              // 30     (11110)
    -1,                                             // 31     (11111)
};

inline int seq_len(char c)
{
	uint8_t high5 = ((uint8_t) c) >> 3;
	return seq_len_table[high5];
}

inline const char* restart(const char* s)
{
	if (*s == 0)
		return s;
	// always increment one byte first, to ensure that we make progress through a series of invalid bytes
	s++;
	for (; *s != 0; s++)
	{
		if (seq_len(*s) != invalid)
			break;
	}
	return s;
}

inline const char* restart(const char* s, const char* end)
{
	if (s >= end)
		return end;
	// always increment one byte first, to ensure that we make progress through a series of invalid bytes
	s++;
	for (; s != end; s++)
	{
		if (seq_len(*s) != invalid)
			break;
	}
	return s;
}

inline bool is_legal_3_byte_code(int cp)
{
	// overlong sequence
	if (cp < min_cp_3)
		return false;

	// UTF-16 surrogate pairs
	if (cp >= utf16_surrogate_low && cp <= utf16_surrogate_high)
		return false;

	// BOM and ffff
	if (cp == invalid_fffe || cp == invalid_ffff)
		return false;

	return true;
}

inline int decode(const char* s, const char* end)
{
	int _seq_len;
	return decode(s, end, _seq_len);
}

inline int decode(const char* s, const char* end, int& _seq_len)
{
	_seq_len = 0;
	int slen = seq_len(s[0]);
	if (slen == invalid)
		return replace;

	if ((intptr_t) (end - s) < (intptr_t) slen)
		return replace;

	int cp = 0;
	switch (slen)
	{
	case 1:
		cp = s[0];
		break;
	case 2:
		if ((s[1] & 0xc0) != 0x80)
			return replace;
		cp = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		if (cp < min_cp_2)
			return replace;
		break;
	case 3:
		if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80)
			return replace;
		cp = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		if (!is_legal_3_byte_code(cp))
			return replace;
		break;
	case 4:
		if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80)
			return replace;
		cp = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		if (cp < min_cp_4 || cp > max4)
			return replace;
		break;
	}
	_seq_len = slen;
	return cp;
}

inline int decode(const char* s)
{
	int _seq_len;
	return decode(s, _seq_len);
}

inline int decode(const char* s, int& _seq_len)
{
	_seq_len = 0;
	int slen = seq_len(s[0]);
	if (slen == invalid)
		return replace;

	int cp = 0;
	switch (slen)
	{
	case 1:
		cp = s[0];
		break;
	case 2:
		if ((s[1] & 0xc0) != 0x80)
			return replace;
		cp = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		if (cp < min_cp_2)
			return replace;
		break;
	case 3:
		if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80)
			return replace;
		cp = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		if (!is_legal_3_byte_code(cp))
			return replace;
		break;
	case 4:
		if ((s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80)
			return replace;
		cp = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		if (cp < min_cp_4 || cp > max4)
			return replace;
		break;
	}
	_seq_len = slen;
	return cp;
}

inline bool next(const char*& s, const char* end, int& cp)
{
	if (s == end)
	{
		cp = replace;
		return false;
	}
	int slen;
	cp = decode(s, end, slen);
	if (cp == replace)
		s = restart(s, end);
	else
		s += slen;
	return true;
}

inline bool next(const char*& s, int& cp)
{
	if (*s == 0)
	{
		cp = replace;
		return false;
	}
	int slen;
	cp = decode(s, slen);
	if (cp == replace)
		s = restart(s);
	else
		s += slen;
	return true;
}

inline int encode(char* buf, int cp)
{
	unsigned ucp = (unsigned) cp;
	if (ucp <= 0x7f)
	{
		buf[0] = ucp;
		return 1;
	}
	else if (ucp <= 0x7ff)
	{
		buf[0] = 0xc0 | (ucp >> 6);
		buf[1] = 0x80 | (ucp & 0x3f);
		return 2;
	}
	else if (ucp <= 0xffff)
	{
		if (!is_legal_3_byte_code(cp))
			return 0;
		buf[0] = 0xe0 | (ucp >> 12);
		buf[1] = 0x80 | ((ucp >> 6) & 0x3f);
		buf[2] = 0x80 | (ucp & 0x3f);
		return 3;
	}
	else if (ucp <= 0x10ffff)
	{
		buf[0] = 0xf0 | (ucp >> 18);
		buf[1] = 0x80 | ((ucp >> 12) & 0x3f);
		buf[2] = 0x80 | ((ucp >> 6) & 0x3f);
		buf[3] = 0x80 | (ucp & 0x3f);
		return 4;
	}
	else
	{
		return 0;
	}
}

inline bool encode(std::string& s, int cp)
{
	char buf[4];
	int  len = encode(buf, cp);
	if (len == 0)
		return false;
	switch (len)
	{
	case 1:
		s += buf[0];
		break;
	case 2:
		s += buf[0];
		s += buf[1];
		break;
	case 3:
		s += buf[0];
		s += buf[1];
		s += buf[2];
		break;
	case 4:
		s += buf[0];
		s += buf[1];
		s += buf[2];
		s += buf[3];
		break;
	default:
		break;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////

inline cp::cp(const char* str, size_t len)
    : Str(str)
{
	if (len == -1)
		End = nullptr;
	else
		End = Str + len;
}

inline cp::cp(const std::string& s)
    : Str(s.c_str()), End(s.c_str() + s.length())
{
}

///////////////////////////////////////////////////////////////////////////////////////////////

inline cp::iter::iter(const char* s, const char* end)
    : S(s), End(end)
{
	if (!known_end() && S != nullptr)
	{
		if (S[0] == 0)
			S = End;
	}
}

inline cp::iter& cp::iter::operator++()
{
	increment();
	return *this;
}

inline cp::iter& cp::iter::operator++(int)
{
	increment();
	return *this;
}

inline void cp::iter::increment()
{
	// Guard against iteration after having reached the end.
	if (S == End)
		return;

	if (known_end())
	{
		int len = seq_len(S[0]);
		if (len == invalid)
		{
			S = restart(S, End);
		}
		else
		{
			if (S + len > End)
				S = End;
			else
				S += len;
		}
	}
	else
	{
		int len = seq_len(S[0]);
		if (len == invalid)
		{
			const char* nextS = restart(S);
			if (nextS == S || nextS[0] == 0)
				S = End;
			else
				S = nextS;
		}
		else
		{
			S += len;
			if ((len == 2 && (S[-1] == 0)) ||
			    (len == 3 && (S[-2] == 0 || S[-1] == 0)) ||
			    (len == 4 && (S[-3] == 0 || S[-2] == 0 || S[-1] == 0)) ||
			    (S[0] == 0))
			{
				S = End;
			}
		}
	}
}

inline int cp::iter::operator*() const
{
	if (known_end())
		return decode(S, End);
	else
		return decode(S);
}

inline bool cp::iter::known_end() const
{
	return End != nullptr;
}
} // namespace utfz

#endif // UTFZ_HPP_INCLUDED