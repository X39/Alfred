#include "global.h"
#include "string_op.h"
#include <ctype.h>
#include <string.h>

#ifndef NULL
	#define NULL 0
#endif
unsigned int str_sw(const char* lString, const char* rString)
{
	unsigned int i;
	char lc, rc;
	for (i = 0; lString[i] != '\0'; i++)
	{
		lc = lString[i];
		rc = rString[i];
		if (rc == '\0')
			return 0;
		if (lc != rc)
			return i + 1;
	}
	return i;
}
unsigned int str_swi(const char* lString, const char* rString)
{
	unsigned int i;
	char lc, rc;
	for (i = 0; lString[i] != '\0'; i++)
	{
		lc = tolower(lString[i]);
		rc = tolower(rString[i]);
		if (rc == '\0')
			return 0;
		if (lc != rc)
			return i + 1;
	}
	return i;
}

unsigned int str_ew(const char* lString, const char* rString)
{
	unsigned int i;
	unsigned int rlen = strlen(rString);
	char lc, rc;
	for (i = 0; lString[i] != '\0'; i++);
	if (i < rlen)
		return 0;
	i -= rlen;
	for (; lString[i] != '\0'; i++)
	{
		lc = lString[i];
		rc = rString[i];
		if (rc == '\0')
			return 1;
		if (lc != rc)
			return 0;
	}
	return 0;
}
unsigned int str_ewi(const char* lString, const char* rString)
{
	unsigned int i;
	unsigned int rlen = strlen(rString);
	char lc, rc;
	for (i = 0; lString[i] != '\0'; i++);
	if (i < rlen)
		return 0;
	i -= rlen;
	for (; lString[i] != '\0'; i++)
	{
		lc = tolower(lString[i]);
		rc = tolower(rString[i]);
		if (rc == '\0')
			return 1;
		if (lc != rc)
			return 0;
	}
	return 0;
}

const char* str_strwrd(const char* lString, const char* rString, const char* letters)
{
	int i, j;
	char lc, rc;
	bool flag = false;
	bool isSeparated = true;

	if (letters == NULL)
		letters = " ,-_\t.?!+:;<>#";
	for (i = 0, j = 0;; i++, j++)
	{
		lc = lString[i];
		rc = rString[j];
		if (flag)
		{
			if (rc == '\0')
			{
				if (chr_is(lc, letters) || lc == '\0')
					return lString + i - j;
				flag = false;
				isSeparated = false;
			}
			if (lc != rc)
			{
				flag = false;
				isSeparated = chr_is(lc, letters);
			}
		}
		else if (!isSeparated)
		{
			if (chr_is(lc, letters))
				isSeparated = true;
			j = -1;
		}
		else if (lc == rc)
		{
			flag = true;
		}
		else
		{
			isSeparated = chr_is(lc, letters);
			j = -1;
		}
		if (lc == '\0')
			return NULL;
	}
}

const char* str_strwrdi(const char* lString, const char* rString, const char* letters)
{
	int i, j;
	char lc, rc;
	bool flag = false;
	bool isSeparated = true;

	if (letters == NULL)
		letters = " ,-_\t";
	for (i = 0, j = 0;; i++, j++)
	{
		lc = tolower(lString[i]);
		rc = tolower(rString[j]);
		if (flag)
		{
			if (rc == '\0')
			{
				if (chr_is(lc, letters) || lc == '\0')
					return lString + i - j;
				flag = false;
				isSeparated = false;
			}
			if (lc != rc)
			{
				flag = false;
				isSeparated = chr_is(lc, letters);
			}
		}
		else if (!isSeparated)
		{
			if (chr_is(lc, letters))
				isSeparated = true;
			j = -1;
		}
		else if (lc == rc)
		{
			flag = true;
		}
		else
		{
			isSeparated = chr_is(lc, letters);
			j = -1;
		}
		if (lc == '\0')
			return NULL;
	}
}

int chr_is(const char c, const char* isArr)
{
	unsigned int i;
	for (i = 0; isArr[i] != '\0'; i++)
	{
		if(isArr[i] == c)
			return 1;
	}
	return 0;
}


unsigned int str_repchr(char* str, char toFind, char toReplace, int length)
{
	unsigned int i, j = 0;
	if (length == -1)
	{
		length = strlen(str);
	}
	for (i = 0; i < length; i++)
	{
		if (str[i] == toFind)
		{
			str[i] = toReplace;
			j++;
		}
	}
	return j;
}