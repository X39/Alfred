#pragma once
/*
Checks if provided string starts with other string.
Returns 0 if true or the index + 1 where the dif was found.

All parameters need to be a \0 terminated string!

param 1: String to check
param 2: String to find
*/
unsigned int str_sw(const char*, const char*);
/*
Checks if provided string ends with other string.
Returns 0 if true or the index + 1 where the dif was found.

All parameters need to be a \0 terminated string!

param 1: String to check
param 2: String to find
*/
unsigned int str_ew(const char*, const char*);
/*
Checks if provided string starts with other string.
Returns 0 if true or the index + 1 where the dif was found.
Comparison is case insensitive.

All parameters need to be a \0 terminated string!

param 1: String to check
param 2: String to find
*/
unsigned int str_swi(const char*, const char*);
/*
Checks if provided string ends with other string.
Returns 0 if true or the index + 1 where the dif was found.
Comparison is case insensitive.

All parameters need to be a \0 terminated string!

param 1: String to check
param 2: String to find
*/
unsigned int str_ewi(const char*, const char*);

/*
Returns NULL ptr if string was not found, ptr to the string if it was

all parameters need to be a \0 terminated string!

param 1: String to check
param 2: String to find
param 3: Characters terminating or NULL for default: " ,-_\t"
*/
const char* str_strwrd(const char*, const char*, const char*);

/*
Returns NULL ptr if string was not found, ptr to the string if it was
Comparison is case insensitive.

all parameters need to be a \0 terminated string!

param 1: String to check
param 2: String to find
param 3: Characters terminating or NULL for default: " ,-_\t.?!+:;<>#"
*/
const char* str_strwrdi(const char*, const char*, const char*);

/*
Returns 1 if provided character is in character array, 0 in any other case.

param 1: Character to check
param 2: Characters which are valid, \0 Terminated
*/
int chr_is(const char, const char*);


/*
Replaces provided char in given string with a different one.
Returns the count of replaced chars.

param 1: String to replace
param 2: Character to search
param 3: Character to replace
param 4: -1 or length of the string
*/
unsigned int str_repchr(char*, char, char, int);