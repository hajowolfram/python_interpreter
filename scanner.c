/*scanner.c*/

//
// Scanner for nuPython programming language. The scanner reads the input
// stream and turns the characters into language Tokens, such as identifiers,
// keywords, and punctuation.
//
// Prof. Joe Hummel (solution)
// Northwestern University
// CS 211
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>  // assert
#include <ctype.h>   // isspace, isdigit, isalpha
#include <stdbool.h> // true, false
#include <string.h>  // strcmp

#include "scanner.h"


//
// collect_identifier
//
// Given the start of an identifier, collects the rest into value
// while advancing the column number.
//
static void collect_identifier(FILE* input, int c, int* colNumber, char* value)
{
  assert(isalpha(c) || c == '_'); // c should be start of identifier

  int i = 0;

  while (isalnum(c) || c == '_') // letter, digit, or underscore
  {
    value[i] = (char)c; // store char
    i++;

    (*colNumber)++; // advance col # past char

    c = fgetc(input); // get next char
  }

  // at this point we found the end of the identifer, so put
  // that last char back for processing later:
  ungetc(c, input);

  // turn the value into a string, and let's see if we have a keyword:
  value[i] = '\0'; // build C-style string:

  return;
}

//
// id_or_keyword
//
// Given an input value, returns whether this value is a
// nuPython keyword or a nuPython identifier. Returns the
// appropriate Token id: nuPy_IDENTIFIER, nuPy_KEYW_AND,
// nuPy_KEYW_BREAK, etc.
//
static int id_or_keyword(char* value)
{
  assert(strlen(value) > 0); // valid value?

  //
  // is the identifier a keyword?
  //
  // NOTE: array elements must be in the SAME ORDER as the
  // keywords in the tokens.h enum.
  //
  char* keywords[] = { "and",   "break", "continue", "def",    "elif", "else",
                      "False", "for",   "if",       "in",     "is",   "None",
                      "not",   "or",    "pass",     "return", "True", "while" };

  int N = sizeof(keywords) / sizeof(keywords[0]);

  int index = -1; // index where we find it, assume not found initially

  for (int i = 0; i < N; i++) {
    if (strcmp(value, keywords[i]) == 0) { // match!
      index = i;
      break;
    }
  }

  if (index < 0)
    return nuPy_IDENTIFIER;
  else
    return nuPy_KEYW_AND + index;
}

//
// collect_numeric_literal
//
// Given the start of an integer or real literal, collects
// the digits (and decimal point) into the given value, advancing
// the column pointer as it goes. The final token ID is returned,
// since it could be an integer literal or a real literal.
//
//
static int collect_numeric_literal(FILE* input, int c, int* colNumber, char* value) 
{
  assert(c == '.' || isdigit(c)); // c should be start of a numeric literal

  int i = 0;

  //
  // if we have a '.', then we should have the start of a 
  // real literal:
  //
  if (c == '.') {
    value[i] = '.';
    i++;

    (*colNumber)++; // advance col # past char

    c = fgetc(input); // get next char

    if (!isdigit(c)) { // '.' by itself => unknown token:
      ungetc(c, input);

      value[i] = '\0';

      return nuPy_UNKNOWN;
    }

    while (isdigit(c)) {
      value[i] = (char)c; // store char
      i++;

      (*colNumber)++; // advance col # past char

      c = fgetc(input); // get next char
    }

    // at this point we found the end of the literal, so put
    // that last char back for processing later:
    ungetc(c, input);

    // turn the value into a string:
    value[i] = '\0'; // build C-style string:

    return nuPy_REAL_LITERAL;
  }

  //
  // if we get here, then we have an initial digit:
  //
  // start collecting the integer part:
  //
  while (isdigit(c)) {
    value[i] = (char)c; // store char
    i++;

    (*colNumber)++; // advance col # past char

    c = fgetc(input); // get next char
  }

  value[i] = '\0'; // build C-style string:

  //
  // we have the integer part, what stopped the loop? if it's
  // not a '.', then we have an integer so return the token:
  //
  if (c != '.') // not a real literal, so return this int:
  {
    ungetc(c, input); // put the char back for processing next:

    return nuPy_INT_LITERAL;
  }

  assert(c == '.'); // we have a real literal:

  //
  // at this point we have the start of a real literal, so
  // collect the decimal point and digits:
  //
  value[i] = '.';
  i++;

  (*colNumber)++; // advance col # past char

  c = fgetc(input); // get next char

  while (isdigit(c)) {
    value[i] = (char)c; // store char
    i++;

    (*colNumber)++; // advance col # past char

    c = fgetc(input); // get next char
  }

  // at this point we found the end of the literal, so put
  // that last char back for processing later:
  ungetc(c, input);

  // turn the value into a string:
  value[i] = '\0'; // build C-style string:

  return nuPy_REAL_LITERAL;
}

//
// collect_string_literal
//
// Given the start of a string literal, collects the rest into value
// while advancing the column number.
//
static void collect_string_literal(FILE* input, int c, int* colNumber, char* value,
  int startLine, int startCol)
{
  assert(c == '"' || c == '\''); // c should be start of string literal

  int startChar = c;

  //
  // we don't want to store the start and end chars, so
  // let's advance past the start of the string literal:
  //
  (*colNumber)++;   // advance col # past char
  c = fgetc(input); // get next char

  //
  // now let's collect the string literal:
  //
  int i = 0;

  while (c != startChar && c != '\n' && c != EOF) {
    value[i] = (char)c; // store char
    i++;

    (*colNumber)++; // advance col # past char

    c = fgetc(input); // get next char
  }

  value[i] = '\0'; // build C-style string:

  //
  // how did the loop end? warn the user if they forgot the
  // end of the string literal:
  //
  if (c == '\n' || c == EOF) {
    printf("**WARNING: string literal @ (%d, %d) not terminated properly\n",
      startLine, startCol);

    ungetc(c, input); // put char back for processing next:
  }
  else {
    //
    // otherwise the string terminated properly, and we consumed
    // the closing quote or double-quote, so advance col number:
    //
    (*colNumber)++; // advance col # past char
  }

  return;
}

//
// scanner_init
//
// Initializes line number, column number, and value before
// the start of the processing the next input stream.
//
void scanner_init(int* lineNumber, int* colNumber, char* value)
{
  assert(lineNumber != NULL);
  assert(colNumber != NULL);
  assert(value != NULL);

  *lineNumber = 1;
  *colNumber = 1;
  value[0] = '\0'; // empty string
}

//
// scanner_nextToken
//
// Returns the next token in the given input stream, advancing the line
// number and column number as appropriate. The token's string-based
// value is returned via the "value" parameter. For example, if the
// token returned is an integer literal, then the value returned is
// the actual literal in string form, e.g. "123". For an identifer,
// the value is the identifer itself, e.g. "print" or "x". For a
// string literal such as 'hi there', the value is the contents of the
// string literal without the quotes.
//
struct Token scanner_nextToken(FILE* input, int* lineNumber, int* colNumber, char* value)
{
  assert(input != NULL);
  assert(lineNumber != NULL);
  assert(colNumber != NULL);
  assert(value != NULL);

  struct Token T;

  //
  // repeatedly input characters one by one until a token is found:
  //
  while (true) {
    //
    // Get the next input character:
    //
    int c = fgetc(input);

    //
    // Let's see what we have...
    //

    if (c == EOF || c == '$') // no more input, return EOS:
    {
      T.id = nuPy_EOS;
      T.line = *lineNumber;
      T.col = *colNumber;

      value[0] = '$';
      value[1] = '\0';

      return T;
    }
    else if (c == '\n') // end of line, keep going:
    {
      (*lineNumber)++; // next line, restart column:
      *colNumber = 1;
      continue;
    }
    else if (isspace(c)) // other form of whitespace, skip:
    {
      (*colNumber)++; // advance col # past char
      continue;
    }
    else if (c == '(') {
      T.id = nuPy_LEFT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == ')') {
      T.id = nuPy_RIGHT_PAREN;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '[') {
      T.id = nuPy_LEFT_BRACKET;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == ']') {
      T.id = nuPy_RIGHT_BRACKET;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '{') {
      T.id = nuPy_LEFT_BRACE;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '}') {
      T.id = nuPy_RIGHT_BRACE;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '+') {
      T.id = nuPy_PLUS;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '-') {
      T.id = nuPy_MINUS;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '/') {
      T.id = nuPy_SLASH;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '%') {
      T.id = nuPy_PERCENT;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '*') {
      //
      // could be * or **, let's assume * for now:
      //
      T.id = nuPy_ASTERISK;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = '*';
      value[1] = '\0';

      //
      // now let's read the next char and see what we have:
      //
      c = fgetc(input);

      if (c == '*') // it's **
      {
        T.id = nuPy_POWER;

        (*colNumber)++; // advance col # past char

        value[1] = '*';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not
      // form a token, so we need to put the char
      // back to be processed on the next call:
      //
      ungetc(c, input);

      return T;
    }
    else if (c == '&') {
      T.id = nuPy_AMPERSAND;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == ':') {
      T.id = nuPy_COLON;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }
    else if (c == '=') {
      //
      // could be = or ==, let's assume = for now:
      //
      T.id = nuPy_EQUAL;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = '=';
      value[1] = '\0';

      //
      // now let's read the next char and see what we have:
      //
      c = fgetc(input);

      if (c == '=') // it's ==
      {
        T.id = nuPy_EQUALEQUAL;

        (*colNumber)++; // advance col # past char

        value[1] = '=';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not
      // form a token, so we need to put the char
      // back to be processed on the next call:
      //
      ungetc(c, input);

      return T;
    }
    else if (c == '!') {
      //
      // could be !=, but we need to confirm, so for now let's
      // assume it's the unknown token if it appears by itself:
      //
      T.id = nuPy_UNKNOWN;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = '!';
      value[1] = '\0';

      //
      // now let's read the next char and see what we have:
      //
      c = fgetc(input);

      if (c == '=') // it's !=
      {
        T.id = nuPy_NOTEQUAL;

        (*colNumber)++; // advance col # past char

        value[1] = '=';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not
      // form a token, so we need to put the char
      // back to be processed on the next call:
      //
      ungetc(c, input);

      return T;
    }
    else if (c == '<') {
      //
      // could be < or <=, let's assume < for now:
      //
      T.id = nuPy_LT;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = '<';
      value[1] = '\0';

      //
      // now let's read the next char and see what we have:
      //
      c = fgetc(input);

      if (c == '=') // it's <=
      {
        T.id = nuPy_LTE;

        (*colNumber)++; // advance col # past char

        value[1] = '=';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not
      // form a token, so we need to put the char
      // back to be processed on the next call:
      //
      ungetc(c, input);

      return T;
    }
    else if (c == '>') {
      //
      // could be > or >=, let's assume > for now:
      //
      T.id = nuPy_GT;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance col # past char

      value[0] = '>';
      value[1] = '\0';

      //
      // now let's read the next char and see what we have:
      //
      c = fgetc(input);

      if (c == '=') // it's >=
      {
        T.id = nuPy_GTE;

        (*colNumber)++; // advance col # past char

        value[1] = '=';
        value[2] = '\0';

        return T;
      }

      //
      // if we get here, then next char did not
      // form a token, so we need to put the char
      // back to be processed on the next call:
      //
      ungetc(c, input);

      return T;
    }
    else if (c == '#') {
      //
      // start of a line comment, we discard the rest of the line:
      //
      while (c != '\n' && c != EOF) {
        (*colNumber)++; // advance col # past char

        c = fgetc(input);
      }

      //
      // the loop has stopped, so either \n or EOF...
      //
      // let's unget and loop around to process the \n or EOF
      // instead of copying the code here:
      //
      ungetc(c, input);

      continue;
    }
    else if (c == '_' || isalpha(c)) {
      //
      // start of identifier or keyword, let's assume identifier for now:
      //
      T.id = nuPy_IDENTIFIER;
      T.line = *lineNumber;
      T.col = *colNumber;

      collect_identifier(input, c, colNumber, value);

      //
      // is the identifier a keyword?
      //
      T.id = id_or_keyword(value);

      return T;
    }
    else if (c == '.' || isdigit(c)) {
      //
      // start of integer or real literal, let's assume integer for
      // now and see what happens:
      //
      T.id = nuPy_INT_LITERAL;
      T.line = *lineNumber;
      T.col = *colNumber;

      T.id = collect_numeric_literal(input, c, colNumber, value);

      return T;
    }
    else if (c == '"' || c == '\'') {
      //
      // start of a string literal...
      //
      // note that literal must start and with the same char...
      //
      T.id = nuPy_STR_LITERAL;
      T.line = *lineNumber;
      T.col = *colNumber;

      collect_string_literal(input, c, colNumber, value, T.line, T.col);

      return T;
    }
    else {
      //
      // if we get here, then char denotes an UNKNOWN token:
      //
      T.id = nuPy_UNKNOWN;
      T.line = *lineNumber;
      T.col = *colNumber;

      (*colNumber)++; // advance past char

      value[0] = (char)c;
      value[1] = '\0';

      return T;
    }

  } // while

  //
  // execution should never get here, return occurs
  // from within loop
  //
}
