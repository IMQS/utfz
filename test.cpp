#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <vector>
#include "utfz.h"

/*

cl /nologo /Zi /EHsc test.cpp utfz.cpp && test
opencppcoverage --sources c:\dev\head\maps\third_party\utfz --modules test.exe -- test.exe

*/

void dump(const char* s)
{
	const char* end = s + strlen(s);
	printf("%s (%d): ", s, utfz::seq_len(s[0]));
	char const* iter = s;
	while (iter != end)
	{
		int cp = utfz::next(iter, end);
		printf("%02x ", cp);
		if (cp == utfz::replace)
			break;
	}

	// test cp iterator on char* with unknown length
	std::vector<int> cp1;
	printf("== ");
	for (auto cp : utfz::cp(s))
	{
		cp1.push_back(cp);
		printf("%02x ", cp);
	}

	// test cp iterator on std::string (ie known length)
	printf("== ");
	std::vector<int> cp2;
	std::string      sz = s;
	for (auto cp : utfz::cp(sz))
	{
		cp2.push_back(cp);
		printf("%02x ", cp);
	}
	assert(cp1.size() == cp2.size());
	for (size_t i = 0; i < cp1.size(); i++)
		assert(cp1[i] == cp2[i]);
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

	int sum = 0;
	double start = clock();
	for (int i = 0; i < 5000; i++)
	{
		for (auto cp : utfz::cp(enc, encEnd - enc))
			sum += cp;
	}
	double end = (clock() - start) / CLOCKS_PER_SEC;

	printf("%-10s, %.0f ms\n", name, end * 1000);

	free(enc);
}

int main(int argc, char** argv)
{
	const char* s1       = "$";  // 1 byte
	const char* s2       = "¢";  // 2 bytes
	const char* s3       = "€";  // 3 bytes
	const char* s4       = "𐍈"; // 4 bytes
	const char* sinvalid = "\x80";
	const char* sempty   = "";

	const int   allsize      = 6;
	const char* all[allsize] = {s1, s2, s3, s4, sinvalid, sempty};
	int         len[allsize] = {1, 2, 3, 4, 0, 0};

	for (int i = 1; i < 100000; i++)
	{
		char buf[10];
		int len = utfz::encode(buf, i);
		int r = utfz::seq_len(buf[0]);
		assert(len == r);
	}

	for (int i = 0; i < allsize; i++)
	{
		dump(all[i]);
		if (len[i] != 0)
		{
			const char* start;
			// make sure buffer overflow checking is correct

			// known length
			start = all[i];
			assert(utfz::next(start, start + len[i]) != utfz::replace);

			start = all[i];
			assert(utfz::next(start, start + len[i] - 1) == utfz::replace);

			// unknown length (null terminated)
			// start on invalid or null
			start = all[i] + 1;
			assert(utfz::next(start) == utfz::replace);

			// truncated code point
			char copy[30];
			strcpy(copy, all[i]);
			copy[len[i] - 1] = 0;
			start            = copy;
			assert(utfz::next(start) == utfz::replace);
		}
	}

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

		// fails when specifying length
		int seq_len = 0;
		int cp      = utfz::decode(enc, enc + 1, seq_len);
		assert(cp == utfz::replace && seq_len == 0);

		// fails if length is unspecified
		cp = utfz::decode(enc);
		assert(cp == utfz::replace);

		// test iterator without length
		std::vector<int> iter1;
		for (auto cp : utfz::cp(enc))
			iter1.push_back(cp);
		assert(iter1.size() == 0);

		// test iterator with length
		std::vector<int> iter2;
		for (auto cp : utfz::cp(enc, 1))
			iter2.push_back(cp);
		assert(iter1.size() == 0);
	}

	{
		// encoding of NUL code point
		char encoded[2];
		assert(utfz::encode(encoded, 0) == 0);
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

		// next_modified on valid input
		const char* s = str;
		assert(utfz::next(s) == 'h');
		assert(s == str + 1);

		// postfix increment
		auto cpiter = utfz::cp(str);
		for (auto cp = cpiter.begin(); cp != cpiter.end(); cp++)
			printf("%d ", *cp);

		// invalid encode to std::string
		std::string ebuf;
		assert(!utfz::encode(ebuf, 0x10ffff + 1));

		// iterator abort for string of known length
		// We need to craft a truncated code point for this.
		//{
		//	std::string z;
		//	z += (char) 0xc0;
		//	auto cp2  = utfz::cp(z);
		//	auto iter = cp2.begin();
		//	assert(iter == cp2.end()); // terminate immediately because first token is truncated
		//}
	}

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

	return 0;
}