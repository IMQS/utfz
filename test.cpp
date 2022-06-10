#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <vector>
#include "utfz.h"

/*

Windows MSVC:
cl /nologo /Zi /EHsc test.cpp utfz.cpp && test
opencppcoverage --sources c:\dev\head\maps\third_party\utfz --modules test.exe -- test.exe

Unix GCC:
g++ -O2 -o test -std=c++11 test.cpp utfz.cpp && ./test

Unix Clang:
clang++ -O2 -o test -std=c++11 test.cpp utfz.cpp && ./test


*/

void test_iterators(const char* s)
{
	const char* end = s + strlen(s);
	printf("%s (%d): ", s, utfz::seq_len(s[0]));

	// iterate using 'next', and with unknown length
	std::vector<int> cp1;
	char const*      iter = s;
	int              cp;
	while (utfz::next(iter, cp))
	{
		cp1.push_back(cp);
		printf("%02x ", cp);
	}

	printf("== ");

	// iterate using 'next', and with known length
	std::vector<int> cp2;
	iter = s;
	while (utfz::next(iter, end, cp))
	{
		cp2.push_back(cp);
		printf("%02x ", cp);
	}

	// test cp iterator on char* with unknown length
	std::vector<int> cp3;
	printf("== ");
	for (auto cp : utfz::cp(s))
	{
		cp3.push_back(cp);
		printf("%02x ", cp);
	}

	// test cp iterator on std::string (ie known length)
	printf("== ");
	std::vector<int> cp4;
	std::string      sz = s;
	for (auto cp : utfz::cp(sz))
	{
		cp4.push_back(cp);
		printf("%02x ", cp);
	}
	assert(cp1.size() == cp2.size());
	assert(cp1.size() == cp3.size());
	assert(cp1.size() == cp4.size());
	for (size_t i = 0; i < cp1.size(); i++)
	{
		assert(cp1[i] == cp2[i]);
		assert(cp1[i] == cp3[i]);
		assert(cp1[i] == cp4[i]);
	}
	printf("\n");
}

void bench(const char* name, int runLength[4])
{
	srand(0);
	int      tokens       = 100000;
	int      encSize      = tokens * 4 + 1;
	char*    enc          = (char*) malloc(encSize);
	char*    encP         = enc;
	unsigned rl           = 3; // index into runLength[]
	int      remain       = 0; // remaining counts inside our current bucket of runLength[]
	int      maxValues[4] = {utfz::max1, utfz::max2, utfz::max3, utfz::max4};
	for (int i = 0; i < tokens; i++)
	{
		while (remain == 0)
		{
			rl     = (rl + 1) % 4;
			remain = runLength[rl];
		}
		unsigned cp = (unsigned) rand();
		cp          = (cp % (maxValues[rl] - 1)) + 1; // the -1, +1 dance is to prevent the zero code point
		int n       = utfz::encode(encP, cp);
		assert(n != 0);
		encP += n;
	}
	char* encEnd = encP;
	assert(encEnd - enc < (intptr_t) encSize);
	*encEnd = 0;

	int    sum   = 0;
	double start = clock();
	for (int i = 0; i < 2000; i++)
	{
		for (auto cp : utfz::cp(enc, encEnd - enc))
			sum += cp;
	}
	double end = (clock() - start) / CLOCKS_PER_SEC;

	printf("%-10s, %.0f ms\n", name, end * 1000);

	free(enc);
}

int encode_any(int cp, char* buf)
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
		buf[0] = 0xe0 | (ucp >> 12);
		buf[1] = 0x80 | ((ucp >> 6) & 0x3f);
		buf[2] = 0x80 | (ucp & 0x3f);
		return 3;
	}
	else if (ucp <= 0x200000)
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

int main(int argc, char** argv)
{
	const char* s1        = "$"; // 1 byte
	const char* s2        = "¢"; // 2 bytes
	const char* s3        = "€"; // 3 bytes
	const char* s4        = "𐍈"; // 4 bytes
	const char* sinvalid1 = "\x80";
	const char* sinvalid2 =
	    "\x80"
	    "a";
	const char* sempty = "";

	const int   allsize      = 7;
	const char* all[allsize] = {s1, s2, s3, s4, sinvalid1, sinvalid2, sempty};
	int         len[allsize] = {1, 2, 3, 4, 1, 2, 0};

	for (int i = 1; i < 50000; i++)
	{
		char buf[10];
		int  len = utfz::encode(buf, i);
		int  r   = utfz::seq_len(buf[0]);
		assert(len == r);
	}

	for (int i = 0; i < allsize; i++)
		test_iterators(all[i]);

	int testCP[] = {1, 0x7f, 0x80, 0x7ff, 0x800, 0xfffd, 0x10000, 0x10ffff};
	for (size_t i = 0; i < sizeof(testCP) / sizeof(testCP[0]); i++)
	{
		char        encoded[5];
		std::string encstr;

		int enc_len      = utfz::encode(encoded, testCP[i]);
		encoded[enc_len] = 0;

		assert(utfz::encode(encstr, testCP[i]));
		assert(strcmp(encoded, encstr.c_str()) == 0);

		int dec_len;
		int cp = utfz::decode(encoded, encoded + enc_len, dec_len);
		assert(dec_len == enc_len);
		assert(cp == testCP[i]);
	}

	{
		// decoding of NUL code point
		const char* enc = "\0";

		// known length
		int seq_len = 0;
		int cp      = utfz::decode(enc, enc + 1, seq_len);
		assert(cp == 0 && seq_len == 1);

		// unknown length
		seq_len = 0;
		cp      = utfz::decode(enc, seq_len);
		assert(cp == 0 && seq_len == 1);

		// test iterator without length
		{
			std::vector<int> iter;
			for (auto cp : utfz::cp(enc))
				iter.push_back(cp);
			assert(iter.size() == 0);
		}

		// test iterator with length
		{
			std::vector<int> iter;
			for (auto cp : utfz::cp(enc, 1))
				iter.push_back(cp);
			assert(iter.size() == 1);
		}
	}

	{
		// encoding of NUL code point
		char encoded[2] = {1, 1};
		assert(utfz::encode(encoded, 0) == 1 && encoded[0] == 0);
	}

	{
		// invalid code point
		char encoded[4];
		assert(utfz::encode(encoded, 0x110000) == 0);

		// Modified UTF-8 decoding
		encoded[0] = (char) 0xc0;
		encoded[1] = (char) 0x80;
		assert(utfz::decode(encoded) == utfz::replace);
		assert(utfz::decode(encoded, encoded + 2) == utfz::replace);

		// detect overlong sequences
		encoded[0] = (char) 0xc0;
		encoded[1] = (char) 0x81;
		assert(utfz::decode(encoded) == utfz::replace);
	}

	{
		// Some arbitrary code to make sure we get 100% code coverage

		const char* str = "h";
		char        buf[10];

		// next_modified on valid input
		const char* s = str;
		int         cp;
		utfz::next(s, cp);
		assert(cp == 'h');
		assert(s == str + 1);

		// postfix increment
		auto cpiter = utfz::cp(str);
		for (auto cp = cpiter.begin(); cp != cpiter.end(); cp++)
			printf("%d ", *cp);

		// invalid encode to std::string
		std::string ebuf;
		assert(!utfz::encode(ebuf, 0x10ffff + 1));

		assert(utfz::restart(str + 1, str) == str);

		// restart on null terminator
		buf[0] = 0;
		assert(utfz::restart(buf) == buf);

		// legality of 3-byte codes
		int slen = 0;
		// overlong
		assert(utfz::decode("\xE0\x01\x01") == utfz::replace);
		// UTF-16 surrogate pairs
		for (int i = 0xd800; i <= 0xdfff; i++)
		{
			assert(encode_any(i, buf) == 3);
			assert(utfz::decode(buf) == utfz::replace);
			assert(utfz::decode(buf, buf + 3) == utfz::replace);
		}
		// two illegal specials (for BOMs)
		assert(encode_any(0xfffe, buf) == 3);
		assert(utfz::decode(buf) == utfz::replace);
		assert(utfz::decode(buf, buf + 3) == utfz::replace);

		assert(encode_any(0xffff, buf) == 3);
		assert(utfz::decode(buf) == utfz::replace);
		assert(utfz::decode(buf, buf + 3) == utfz::replace);

		// too large
		assert(encode_any(0x10ffff, buf) == 4);
		assert(utfz::decode(buf) == 0x10ffff);
		assert(utfz::decode(buf, buf + 4) == 0x10ffff);
		assert(encode_any(0x10ffff + 1, buf) == 4);
		assert(utfz::decode(buf) == utfz::replace);
		assert(utfz::decode(buf, buf + 4) == utfz::replace);

		// highest two bits in subsequent bytes are not '10'; next plausible decode point is one byte onwards
		const char* badHigh[6] = {
		    // bit 7 is not 1 (3F is the illegal byte here)
		    "\xC4\x3F",
		    "\xE4\xB0\x3F",
		    "\xF4\xB0\xB0\x3F",
		    // bit 6 is not 0 (F0 is the illegal byte here)
		    "\xC4\xF0",
		    "\xE4\xB0\xF0",
		    "\xF4\xB0\xB0\xF0",
		};
		for (int i = 0; i < 6; i++)
		{
			const char* s   = badHigh[i];
			const char* end = s + strlen(s);
			assert(utfz::decode(s, slen) == utfz::replace);
			assert(utfz::decode(s, end, slen) == utfz::replace);
			assert(utfz::restart(s) > s);
			assert(utfz::restart(s, end) > s);
		}

		// illegal encoding
		assert(utfz::encode(buf, 0xfffe) == 0);

		// decode truncated code point with known length
		assert(encode_any(0xd123, buf) == 3);
		assert(utfz::decode(buf, buf + 2) == utfz::replace);

		// decode truncated code points with unknown length
		assert(encode_any(utfz::max1 + 1, buf) == 2);
		buf[1] = 0;
		assert(utfz::decode(buf) == utfz::replace);

		assert(encode_any(utfz::max2 + 1, buf) == 3);
		buf[2] = 0;
		assert(utfz::decode(buf) == utfz::replace);

		assert(encode_any(utfz::max3 + 1, buf) == 4);
		buf[3] = 0;
		assert(utfz::decode(buf) == utfz::replace);

		// iterate on empty string
		const char* empty = "";
		auto        iter  = utfz::cp(empty).begin();
		auto        end   = utfz::cp(empty).end();
		iter++;
		iter++;
		assert(iter == end);
		iter = utfz::cp(empty, 0).begin();
		end  = utfz::cp(empty, 0).end();
		iter++;
		iter++;
		assert(iter == end);

		// iterate on truncated string (known length)
		assert(encode_any(utfz::max3 + 1, buf) == 4);
		iter = utfz::cp(buf, 3).begin();
		end  = utfz::cp(buf, 3).end();
		assert(iter != end);
		assert(*iter == utfz::replace);
		iter++;
		assert(iter == end);

		// iterate on truncated string (unknown length)
		assert(encode_any(utfz::max3 + 1, buf) == 4);
		buf[3] = 0;
		iter   = utfz::cp(buf).begin();
		end    = utfz::cp(buf).end();
		assert(iter != end);
		assert(*iter == utfz::replace);
		iter++;
		assert(iter == end);
	}

#ifndef _DEBUG
	{
		// speed
		printf("\n");
		int ascii[4] = {1, 0, 0, 0};
		int low[4]   = {10, 2, 1, 0};
		int high[4]  = {1, 1, 1, 1};
		bench("ascii", ascii);
		bench("low", low);
		bench("high", high);
	}
#endif

	return 0;
}
