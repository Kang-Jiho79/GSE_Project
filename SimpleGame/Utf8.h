#pragma once

#include <string>
#include <vector>

// UTF-8 to code points and back, for text that has to be taken apart a
// character at a time - written out letter by letter, or broken up.
namespace Utf8
{
	inline void Decode(const char* text, std::vector<unsigned int>& out)
	{
		out.clear();
		const unsigned char* p = (const unsigned char*)text;
		while (*p != 0)
		{
			unsigned int c = *p;
			int extra = 0;
			if (c >= 0xF0)
			{
				c &= 0x07;
				extra = 3;
			}
			else if (c >= 0xE0)
			{
				c &= 0x0F;
				extra = 2;
			}
			else if (c >= 0xC0)
			{
				c &= 0x1F;
				extra = 1;
			}
			p++;
			for (int i = 0; i < extra && (*p & 0xC0) == 0x80; i++)
			{
				c = (c << 6) | (*p & 0x3F);
				p++;
			}
			out.push_back(c);
		}
	}

	inline void Append(std::string& out, unsigned int c)
	{
		if (c < 0x80)
		{
			out += (char)c;
		}
		else if (c < 0x800)
		{
			out += (char)(0xC0 | (c >> 6));
			out += (char)(0x80 | (c & 0x3F));
		}
		else if (c < 0x10000)
		{
			out += (char)(0xE0 | (c >> 12));
			out += (char)(0x80 | ((c >> 6) & 0x3F));
			out += (char)(0x80 | (c & 0x3F));
		}
		else
		{
			out += (char)(0xF0 | (c >> 18));
			out += (char)(0x80 | ((c >> 12) & 0x3F));
			out += (char)(0x80 | ((c >> 6) & 0x3F));
			out += (char)(0x80 | (c & 0x3F));
		}
	}
}
