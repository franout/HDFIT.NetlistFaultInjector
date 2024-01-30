#include "common.h"

#include <limits.h>



const char *ParseUtils::strrstr(const char *haystack, const char *pos, const char *needle)
{
	const size_t needleLen = strlen(needle);

	if (needleLen > pos - haystack)
	{
		return nullptr;
	}

	const char *currPos = pos - needleLen;
	while (currPos - needleLen > haystack)
	{
		if (0 == strncmp(currPos, needle, needleLen))
		{
			return currPos;
		}
		currPos--;
	}

	return nullptr;
}

void ParseUtils::backslashToDoubleBackslash(std::string *out, const std::string &in)
{
	out->resize(0);
	out->reserve(2 * in.size());

	for (const auto &elem : in)
	{
		if ('\\' == elem)
		{
			out->append("\\\\");
		}
		else
		{
			out->push_back(elem);
		}
	}
}

bool ParseUtils::isSpace(char c)
{
	return (' ' == c) || ('\t' == c);
}

const char *ParseUtils::firstNonSpaceGet(const char *in)
{
	const char *out = in;
	while (isSpace(*out))
	{
		out++;
	}

	return out;
}

const char *ParseUtils::lastNonSpaceGet(const char *in)
{
	const char *out = in;
	while (isSpace(*out))
	{
		out--;
	}

	return out;
}

const char *ParseUtils::lastCharAfterGet(const char *in, const char *pNeedles, size_t nNeedles)
{
	const char *out = in;

	while (*out)
	{
		for (size_t needle = 0; needle < nNeedles; needle++)
		{
			if (*out == pNeedles[needle])
			{
				return out + 1;
			}
		}

		out--;
	}

	return out;
}


int ParseUtils::signalWidthGet(const char ** endWidth, const char * in)
{
	if('[' != in[0])
	{
		nfiError("First char expected to be [\n");
		return -1;
	}

	char * widthColon;
	long widthHigh = strtol(in + 1, &widthColon, 10);
	if(in + 1 == widthColon)
	{
		nfiError("Found no number\n");
		return -1;
	}

	if((LONG_MIN == widthHigh) || (LONG_MAX == widthHigh))
	{
		nfiError("Width doesn't fit long\n");
		return -1;
	}

	if(':' != *widthColon)
	{
		nfiError("Expected colon after width high: %.30s\n", in);
		return -1;
	}

	char * widthEnd;
	long widthLow = strtol(widthColon + 1, &widthEnd, 10);
	if(widthColon + 1 == widthEnd)
	{
		nfiError("Found no number\n");
		return -1;
	}

	if((LONG_MIN == widthLow) || (LONG_MAX == widthLow))
	{
		nfiError("Width doesn't fit long\n");
		return -1;
	}

	if(']' != *widthEnd)
	{
		nfiError("Expected ] after width low: %.30s\n", in);
		return -1;
	}

	*endWidth = widthEnd;

	return abs(widthHigh - widthLow) + 1;
}

// returns <= 0 on error, else signal width
int ParseUtils::signalArraySizeGet(const char ** endArray, const char * in)
{
	if('[' != in[0])
	{
		nfiError("First char expected to be [\n");
		return -1;
	}

	char * widthHighEnd;
	long widthHigh = strtol(in + 1, &widthHighEnd, 10);
	if(in + 1 == widthHighEnd)
	{
		nfiError("Found no number\n");
		return -1;
	}

	if((LONG_MIN == widthHigh) || (LONG_MAX == widthHigh))
	{
		nfiError("Width doesn't fit long\n");
		return -1;
	}

	if(']' == *widthHighEnd)
	{
		// System Verilog array declaration 'signal[arraySize]'
		*endArray = widthHighEnd;
		return widthHigh;
	}

	if(':' != *widthHighEnd)
	{
		nfiError("Expected colon after width high: %.30s\n", in);
		return -1;
	}

	char * widthEnd;
	long widthLow = strtol(widthHighEnd + 1, &widthEnd, 10);
	if(widthHighEnd + 1 == widthEnd)
	{
		nfiError("Found no number\n");
		return -1;
	}

	if((LONG_MIN == widthLow) || (LONG_MAX == widthLow))
	{
		nfiError("Width doesn't fit long\n");
		return -1;
	}

	if(']' != *widthEnd)
	{
		nfiError("Expected ] after width low: %.30s\n", in);
		return -1;
	}

	*endArray = widthEnd;

	return abs(widthHigh - widthLow) + 1;
}



int ParseUtils::subSignalArraySizeGet(const char ** endArray, const char * in)
{
	if('[' != in[0])
	{
		nfiError("First char expected to be [\n");
		return -1;
	}

	char * widthHighEnd;
	long widthHigh = strtol(in + 1, &widthHighEnd, 10);
	if(in + 1 == widthHighEnd)
	{
		nfiError("Found no number\n");
		return -1;
	}

	if((LONG_MIN == widthHigh) || (LONG_MAX == widthHigh))
	{
		nfiError("Width doesn't fit long\n");
		return -1;
	}

	if(']' == *widthHighEnd)
	{
		*endArray = widthHighEnd;
		return 1; // only one element chosen
	}

	if(':' != *widthHighEnd)
	{
		nfiError("Expected colon after width high: %.30s\n", in);
		return -1;
	}

	char * widthEnd;
	long widthLow = strtol(widthHighEnd + 1, &widthEnd, 10);
	if(widthHighEnd + 1 == widthEnd)
	{
		nfiError("Found no number\n");
		return -1;
	}

	if((LONG_MIN == widthLow) || (LONG_MAX == widthLow))
	{
		nfiError("Width doesn't fit long\n");
		return -1;
	}

	if(']' != *widthEnd)
	{
		nfiError("Expected ] after width low: %.30s\n", in);
		return -1;
	}

	*endArray = widthEnd;

	return abs(widthHigh - widthLow) + 1;
}



bool ParseUtils::has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}