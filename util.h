/*util.h*/

//
// Utility functions for nuPython
//
// Prof. Joe Hummel
// Northwestern University
// CS 211
//

#pragma once


//
// dupString
// 
// Duplicates the given string and returns a pointer
// to the copy.
// 
// NOTE: this function allocates memory for the copy,
// the caller takes ownership of the copy and must
// eventually free that memory.
//
char* dupString(char* s);

//
// dupAndStripEOLN
// 
// Duplicates the given string and returns a pointer
// to the copy; any EOLN characters (\n, \r, etc.)
// are also removed.
// 
// NOTE: this function allocates memory for the copy,
// the caller takes ownership of the copy and must
// eventually free that memory.
//
char* dupAndStripEOLN(char* s);

//
// icmpStrings
//
// case-insensitive comparison of strings s1 and s2.
// Like strcmp, returns 0 if s1 == s2 and returns a
// non-zero value if s1 != s2.
//
// Example: icmpStrings("apple", "APPLE") returns 0
//
int icmpStrings(char* s1, char* s2);
