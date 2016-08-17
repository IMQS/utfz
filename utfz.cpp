#include "utfz.h"
#include <stdint.h>
#include <string.h>

namespace utfz {

template<bool Modified>
int T_seq_len(char c)
{
	if (Modified)
	{
		// This is the only place where we enforce the use of "Modified UTF-8",
		// as described in the documentation inside utfz.h
		if (c == 0)
			return 0;
	}

	if ((c & 0x80) == 0)
		return 1;

	if ((c & 0xe0) == 0xc0)
		return 2;

	if ((c & 0xf0) == 0xe0)
		return 3;

	if ((c & 0xf8) == 0xf0)
		return 4;

	return invalid;
}

int seq_len(char c)
{
	return T_seq_len<false>(c);
}

int seq_len_modified(char c)
{
	return T_seq_len<true>(c);
}

int decode(const char* s, const char* end)
{
	int _seq_len;
	return decode(s, end, _seq_len);
}

int decode(const char* s, const char* end, int& _seq_len)
{
	int slen = seq_len(s[0]);
	if (slen == invalid)
		return invalid;

	if ((intptr_t)(end - s) < (intptr_t) slen)
		return invalid;

	int cp = 0;
	switch (slen)
	{
	case 1:
		cp = s[0];
		break;
	case 2:
		cp = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		break;
	case 3:
		cp = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		break;
	case 4:
		cp = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
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
	int slen = seq_len_modified(s[0]);
	if (slen == 0 || slen == invalid)
		return invalid;

	int cp = 0;
	switch (slen)
	{
	case 1:
		cp = s[0];
		break;
	case 2:
		if (s[1] == 0)
			return invalid;
		cp = ((s[0] & 0x1f) << 6) | (s[1] & 0x3f);
		break;
	case 3:
		if (s[1] == 0 || s[2] == 0)
			return invalid;
		cp = ((s[0] & 0xf) << 12) | ((s[1] & 0x3f) << 6) | (s[2] & 0x3f);
		break;
	case 4:
		if (s[1] == 0 || s[2] == 0 || s[3] == 0)
			return invalid;
		cp = ((s[0] & 0x7) << 18) | ((s[1] & 0x3f) << 12) | ((s[2] & 0x3f) << 6) | (s[3] & 0x3f);
		break;
	}
	_seq_len = slen;
	return cp;
}

int next(const char*& s, const char* end)
{
	int slen;
	int cp = decode(s, end, slen);
	if (cp != invalid)
		s += slen;
	return cp;
}

int next(const char*& s)
{
	int slen;
	int cp = decode(s, slen);
	if (cp != invalid)
		s += slen;
	return cp;
}

int encode(char* buf, int cp)
{
	unsigned ucp = (unsigned) cp;
	if (ucp == 0)
	{
		// Always output "Modified UTF-8"
		buf[0] = (char) 0xc0;
		buf[1] = (char) 0x80;
		return 2;
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
	int len = encode(buf, cp);
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
	check_first();
}

void cp::iter::check_first()
{
	increment<true>();
}

cp::iter& cp::iter::operator++()
{
	increment<false>();
	return *this;
}

cp::iter& cp::iter::operator++(int)
{
	increment<false>();
	return *this;
}

template<bool is_check>
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
			S = End;
		}
		else
		{
			if (S + len > End)
			{
				S = End;
			}
			else
			{
				if (!is_check)
					S += len;
			}
		}
	}
	else
	{
		int len = seq_len_modified(S[0]);
		if (len == invalid || len == 0)
		{
			S = End;
		}
		else
		{
			if (!is_check)
			{
				S += len;
				if (S[0] == 0)
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
