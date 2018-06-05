#ifndef OBJECTS_STRING_H
#define OBJECTS_STRING_H

#include <stdarg.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep
#include "../unicode.h"        // IWYU pragma: keep

// bad things happen if s is not a string object or i < STRINGOBJECT_LEN(s)
// otherwise these never fail
// these DO NOT return a new reference!
#define STRINGOBJECT_GET(s, i) (((struct UnicodeString*) (s)->data)->val[(i)])
#define STRINGOBJECT_LEN(s)    (((struct UnicodeString*) (s)->data)->len)

// RETURNS A NEW REFERENCE or NULL on no mem, see builtins_setup()
struct Object *stringobject_createclass_noerr(struct Interpreter *interp);

// should be called when other stuff is set up, see builtins_setup()
// returns STATUS_OK or STATUS_ERROR
int stringobject_addmethods(struct Interpreter *interp);

// makes a copy of ustr with unicodestring_copy(), returns NULL on error
// RETURNS A NEW REFERENCE
struct Object *stringobject_newfromustr(struct Interpreter *interp, struct UnicodeString ustr);

// ptr must be \0-terminated
// RETURNS A NEW REFERENCE
struct Object *stringobject_newfromcharptr(struct Interpreter *interp, char *ptr);

/* create a new string kinda like printf

fmt must be valid UTF-8, and it can contain any of these format specifiers:

	             ,---- cast if needed
	             |
	             V
	SPECIFIER  EXACT TYPE             NOTES
	----------------------------------------------------------------------------------
	%s         char *                 an error is set for invalid utf8
	%U         struct UnicodeString
	%S         struct Object *        to_string will be called
	%D         struct Object *        to_debug_string will be called
	%p         void *                 prints the pointer in the output, e.g. 0x1b6baa0
	%L         long long
	%%         nothing                a % in the output

nothing else works, not even padding like %5s

these fixed limits are NOT CHECKED even with assert:
	* stuff between format specifiers can be at most 200 characters long
	* number of format specifiers + number of texts between them must be <= 20
*/
struct Object *stringobject_newfromfmt(struct Interpreter *interp, char *fmt, ...);

// like newfromfmt, but vprintf style
struct Object *stringobject_newfromvfmt(struct Interpreter *interp, char *fmt, va_list ap);

// the returned UnicodeString's val must NOT be free()'d
struct UnicodeString *stringobject_getustr(struct Object *s);

// returns an array of substrings or NULL on error
// bad things happen if the string is not a string object
struct Object *stringobject_splitbywhitespace(struct Interpreter *interp, struct Object *s);


#endif   // OBJECTS_STRING_H
