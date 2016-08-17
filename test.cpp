#include <stdio.h>
#include <string.h>
#include <assert.h>
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
	char const * iter = s;
	while (iter != end)
	{
		int cp = utfz::next(iter, end);
		printf("%02x ", cp);
		if (cp == utfz::invalid)
			break;
	}
	
	// test cp iterator on char* with unknown length
	printf("== ");
	for (auto cp : utfz::cp(s))
		printf("%02x ", cp);
	
	// test cp iterator on std::string (ie known length)
	printf("== ");
	std::string sz = s;
	for (auto cp : utfz::cp(sz))
		printf("%02x ", cp);
	printf("\n");
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

	for (int i = 0; i < allsize; i++)
	{
		dump(all[i]);
		if (len[i] != 0)
		{
			const char* start;
			// make sure buffer overflow checking is correct

			// known length
			start = all[i];
			assert(utfz::next(start, start + len[i]) != utfz::invalid);

			start = all[i];
			assert(utfz::next(start, start + len[i] - 1) == utfz::invalid);

			// unknown length (null terminated)
			// start on invalid or null
			start = all[i] + 1;
			assert(utfz::next(start) == utfz::invalid);
			
			// truncated code point
			char copy[30];
			strcpy(copy, all[i]);
			copy[len[i] - 1] = 0;
			start = copy;
			assert(utfz::next(start) == utfz::invalid);
		}
	}

	int testCP[] = {0, 1, 0x7f, 0x80, 0x7ff, 0x800, 0xffff, 0x10000, 0x10ffff};
	for (size_t i = 0; i < sizeof(testCP) / sizeof(testCP[0]); i++)
	{
		char encoded[5];
		std::string encstr;
		
		int enc_len = utfz::encode(encoded, testCP[i]);
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
		
		// success when specifying length
		int seq_len = 0;
		int cp = utfz::decode(enc, enc + 1, seq_len);
		assert(cp == 0 && seq_len == 1);
		
		// fails if length is unspecified
		cp = utfz::decode(enc);
		assert(cp == utfz::invalid);

		// test iterator. Fails because we do not specify length
		std::vector<int> iter1;
		for (auto cp : utfz::cp(enc))
			iter1.push_back(cp);
		assert(iter1.size() == 0);

		// test iterator. Succeeds, because we specify length
		std::vector<int> iter2;
		for (auto cp : utfz::cp(enc, 1))
			iter2.push_back(cp);
		assert(iter2.size() == 1 && iter2[0] == 0);
	}

	{
		// encoding of NUL code point
		char encoded[2];
		assert(utfz::encode(encoded, 0) == 2);
		assert(encoded[0] == (char) 0xc0);
		assert(encoded[1] == (char) 0x80);
	}

	{
		// invalid code point
		char encoded[4];
		assert(utfz::encode(encoded, 0x110000) == 0);
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
		{
			std::string z;
			z += (char) 0xc0;
			auto cp2 = utfz::cp(z);
			auto iter = cp2.begin();
			assert(iter == cp2.end()); // terminate immediately because first token is truncated
		}
	}

	return 0;
}