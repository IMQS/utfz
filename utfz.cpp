// -----------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.
// -----------------------------------------------------------------------
#include "utfz.h"
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

int seq_len(char c)
{
	// We do not allow the U+0000 code point
	if (c == 0)
		return invalid;

	uint8_t high5 = ((uint8_t) c) >> 3;
	return seq_len_table[high5];
}

const char* restart(const char* s)
{
	for (; *s != 0; s++)
	{
		if (seq_len(*s) != invalid)
			break;
	}
	return s;
}

const char* restart(const char* s, const char* end)
{
	if (s >= end)
		return end;
	for (; s != end; s++)
	{
		if (seq_len(*s) != invalid)
			break;
	}
	return s;
}

static bool is_legal_3_byte_code(int cp)
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

int decode(const char* s, const char* end)
{
	int _seq_len;
	return decode(s, end, _seq_len);
}

int decode(const char* s, const char* end, int& _seq_len)
{
	_seq_len = 0;
	int slen = seq_len(s[0]);
	if (slen == invalid)
		return replace;

	if ((intptr_t)(end - s) < (intptr_t) slen)
		return replace;

	int cp = 0;
	switch (slen)
	{
	case 1:
		cp = s[0];
		break;
	case 2:
		cp = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		if (cp < min_cp_2)
			return replace;
		break;
	case 3:
		cp = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		if (!is_legal_3_byte_code(cp))
			return replace;
		break;
	case 4:
		cp = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		if (cp < min_cp_4 || cp > max4)
			return replace;
		break;
	}
	_seq_len = slen;
	return cp;
}

int decode(const char* s)
{
	int _seq_len;
	return decode(s, _seq_len);
}

int decode(const char* s, int& _seq_len)
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
		if (s[1] == 0)
			return replace;
		cp = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		if (cp < min_cp_2)
			return replace;
		break;
	case 3:
		if (s[1] == 0 || s[2] == 0)
			return replace;
		cp = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		if (!is_legal_3_byte_code(cp))
			return replace;
		break;
	case 4:
		if (s[1] == 0 || s[2] == 0 || s[3] == 0)
			return replace;
		cp = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		if (cp < min_cp_4 || cp > max4)
			return replace;
		break;
	}
	_seq_len = slen;
	return cp;
}

bool next(const char*& s, const char* end, int& cp)
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

bool next(const char*& s, int& cp)
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

int encode(char* buf, int cp)
{
	unsigned ucp = (unsigned) cp;
	if (ucp == 0)
	{
		// we do not support encoding or decoding U+0000
		return 0;
	}
	else if (ucp <= 0x7f)
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

bool encode(std::string& s, int cp)
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

cp::cp(const char* str, size_t len)
    : Str(str)
{
	if (len == -1)
		End = nullptr;
	else
		End = Str + len;
}

cp::cp(const std::string& s)
    : Str(s.c_str()), End(s.c_str() + s.length())
{
}

///////////////////////////////////////////////////////////////////////////////////////////////

cp::iter::iter(const char* s, const char* end)
    : S(s), End(end)
{
	if (!known_end() && S != nullptr)
	{
		if (S[0] == 0)
			S = End;
	}
}

cp::iter& cp::iter::operator++()
{
	increment();
	return *this;
}

cp::iter& cp::iter::operator++(int)
{
	increment();
	return *this;
}

void cp::iter::increment()
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

int cp::iter::operator*() const
{
	if (known_end())
		return decode(S, End);
	else
		return decode(S);
}

bool cp::iter::known_end() const
{
	return End != nullptr;
}
}
