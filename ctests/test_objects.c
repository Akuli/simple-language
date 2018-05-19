#include <src/common.h>
#include <src/interpreter.h>
#include <src/method.h>
#include <src/objects/array.h>
#include <src/objects/classobject.h>
#include <src/objects/errors.h>
#include <src/objects/function.h>
#include <src/objects/integer.h>
#include <src/objects/string.h>
#include <src/objectsystem.h>
#include <src/unicode.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"


int cleaner_ran = 0;
void cleaner(struct Object *obj)
{
	buttert(obj->data == (void *)0xdeadbeef);
	cleaner_ran++;
}

void test_objects_simple(void)
{
	buttert(cleaner_ran == 0);
	struct Object *objectclass = interpreter_getbuiltin(testinterp, NULL, "Object");
	buttert(objectclass);
	struct Object *obj = classobject_newinstance(testinterp, NULL, objectclass, (void *)0xdeadbeef, cleaner);
	OBJECT_DECREF(testinterp, objectclass);
	buttert(obj);
	buttert(obj->data == (void *)0xdeadbeef);
	buttert(cleaner_ran == 0);
	OBJECT_DECREF(testinterp, obj);
	buttert(cleaner_ran == 1);
}

void test_objects_error(void)
{
	struct Object *err = NULL;
	errorobject_setwithfmt(testinterp, &err, "oh %s", "shit");
	buttert(err);
	struct UnicodeString *msg = ((struct Object*) err->data)->data;
	buttert(msg);
	buttert(msg->len = 7);
	buttert(
		msg->val[0] == 'o' &&
		msg->val[1] == 'h' &&
		msg->val[2] == ' ' &&
		msg->val[3] == 's' &&
		msg->val[4] == 'h' &&
		msg->val[5] == 'i' &&
		msg->val[6] == 't');
	OBJECT_DECREF(testinterp, err);
}

struct Object *callback_arg1, *callback_arg2;
int flipped = 0;

struct Object *callback(struct Interpreter *interp, struct Object **errptr, struct Object **args, size_t nargs)
{
	buttert(nargs == 2);
	buttert(args[0] == (flipped ? callback_arg2 : callback_arg1));
	buttert(args[1] == (flipped ? callback_arg1 : callback_arg2));
	return (struct Object*) 0x123abc;
}

void test_objects_function(void)
{
	buttert((callback_arg1 = stringobject_newfromcharptr(testinterp, NULL, "asd1")));
	buttert((callback_arg2 = stringobject_newfromcharptr(testinterp, NULL, "asd2")));

	struct Object *func = functionobject_new(testinterp, NULL, callback);
	buttert(functionobject_call(testinterp, NULL, func, callback_arg1, callback_arg2, NULL) == (struct Object*) 0x123abc);

	struct Object *partial1 = functionobject_newpartial(testinterp, NULL, func, callback_arg1);
	OBJECT_DECREF(testinterp, callback_arg1);   // partialfunc should hold a reference to this
	OBJECT_DECREF(testinterp, func);
	flipped = 0;
	buttert(functionobject_call(testinterp, NULL, partial1, callback_arg2, NULL) == (struct Object*) 0x123abc);

	struct Object *partial2 = functionobject_newpartial(testinterp, NULL, partial1, callback_arg2);
	OBJECT_DECREF(testinterp, callback_arg2);
	OBJECT_DECREF(testinterp, partial1);
	flipped = 1;    // arg 2 was partialled last, so it will go first
	buttert(functionobject_call(testinterp, NULL, partial2, NULL) == (struct Object*) 0x123abc);

	OBJECT_DECREF(testinterp, partial2);
}

#define ODOTDOT 0xd6    // Ö
#define odotdot 0xf6    // ö

void test_objects_string(void)
{
	struct UnicodeString u;
	u.len = 2;
	u.val = bmalloc(sizeof(unicode_char) * 2);
	u.val[0] = ODOTDOT;
	u.val[1] = odotdot;

	struct Object *strs[] = {
		stringobject_newfromcharptr(testinterp, NULL, "Öö"),
		stringobject_newfromustr(testinterp, NULL, u) };
	free(u.val);    // must not break anything, newfromustr should copy

	for (size_t i=0; i < sizeof(strs)/sizeof(strs[0]); i++) {
		buttert(strs[i]);
		struct UnicodeString *data = strs[i]->data;
		buttert(data);
		buttert(data->len == 2);
		buttert(data->val[0] == ODOTDOT);
		buttert(data->val[1] == odotdot);
		OBJECT_DECREF(testinterp, strs[i]);
	}
}

void test_objects_string_tostring(void)
{
	struct Object *s = stringobject_newfromcharptr(testinterp, NULL, "Öö");
	buttert(s);
	struct Object *ret = method_call(testinterp, NULL, s, "to_string", NULL);
	buttert(ret);
	buttert(ret == s);
	OBJECT_DECREF(testinterp, s);    // functionobject_call() returned a new reference
	OBJECT_DECREF(testinterp, s);    // stringobject_newfromustr() returned a new reference
}

void test_objects_string_newfromfmt(void)
{
	unicode_char bval = 'b';
	struct UnicodeString b;
	b.len = 1;
	b.val = &bval;

	struct Object *c = stringobject_newfromcharptr(testinterp, NULL, "c");
	buttert(c);

	struct Object *res = stringobject_newfromfmt(testinterp, NULL, "-%s-%U-%S-%D-%%-", "a", b, c, c);
	buttert(res);
	OBJECT_DECREF(testinterp, c);

	struct UnicodeString *s = res->data;
	buttert(s->len == 13 /* OMG BAD LUCK */);
	buttert(
		s->val[0] == '-' &&
		s->val[1] == 'a' &&
		s->val[2] == '-' &&
		s->val[3] == 'b' &&
		s->val[4] == '-' &&
		s->val[5] == 'c' &&
		s->val[6] == '-' &&
		s->val[7] == '"' &&
		s->val[8] == 'c' &&
		s->val[9] == '"' &&
		s->val[10] == '-' &&
		s->val[11] == '%' &&
		s->val[12] == '-');
	OBJECT_DECREF(testinterp, res);
}

void test_objects_array(void)
{
	struct Object *objs[] = {
		stringobject_newfromcharptr(testinterp, NULL, "a"),
		stringobject_newfromcharptr(testinterp, NULL, "b"),
		stringobject_newfromcharptr(testinterp, NULL, "c") };
#define NOBJS (sizeof(objs) / sizeof(objs[0]))
	for (size_t i=0; i < NOBJS; i++)
		buttert(objs[i]);

	struct Object *arr = arrayobject_new(testinterp, NULL, objs, NOBJS - 1);
	buttert(arr);
	buttert(arrayobject_push(testinterp, NULL, arr, objs[NOBJS-1]) == STATUS_OK);

	// now the array should hold references to each object
	for (size_t i=0; i < NOBJS; i++)
		OBJECT_DECREF(testinterp, objs[i]);

	struct ArrayObjectData *data = arr->data;
	buttert(data->len == NOBJS);
	for (size_t i=0; i < NOBJS; i++) {
#undef NOBJS
		buttert(data->elems[i] == objs[i]);
		struct UnicodeString *ustr = ((struct Object *) data->elems[i])->data;
		buttert(ustr->len == 1);
		buttert(ustr->val[0] == 'a' + i);
	}
	OBJECT_DECREF(testinterp, arr);
}

#define HOW_MANY 1000
static void check(struct ArrayObjectData *arrdata)
{
	buttert(arrdata);
	buttert(arrdata->len == HOW_MANY);
	for (size_t i=0; i < HOW_MANY; i++) {
		buttert(arrdata->elems[i]->refcount == 2);
		buttert(integerobject_tolonglong(arrdata->elems[i]) == (long long) (i + 10));
	}
}

void test_objects_array_many_elems(void)
{
	struct Object *objs[HOW_MANY];
	for (size_t i=0; i < HOW_MANY; i++)
		buttert((objs[i] = integerobject_newfromlonglong(testinterp, NULL, i+10)));

	struct Object *arr = arrayobject_new(testinterp, NULL, objs, HOW_MANY);
	check(arr->data);

	for (int i = HOW_MANY-1; i >= 0; i--) {
		struct Object *obj = arrayobject_pop(testinterp, arr);
		buttert(obj == objs[i]);
	}

	for (size_t i=0; i < HOW_MANY; i++)
		buttert(arrayobject_push(testinterp, NULL, arr, objs[i]) == STATUS_OK);
	check(arr->data);

	OBJECT_DECREF(testinterp, arr);
	for (size_t i=0; i < HOW_MANY; i++)
		OBJECT_DECREF(testinterp, objs[i]);
}
#undef HOW_MANY

struct HashTest {
	struct Object *obj;
	int shouldBhashable;
};

void test_objects_hashes(void)
{
	struct Object *err;
	errorobject_setwithfmt(testinterp, &err, "oh %s", "shit");
	struct Object *objectclass = interpreter_getbuiltin(testinterp, NULL, "Object");
	struct HashTest tests[] = {
		{ interpreter_getbuiltin(testinterp, NULL, "String") /* a class */, 1 },
		{ err, 1 },
		{ interpreter_getbuiltin(testinterp, NULL, "print"), 1 },
		{ integerobject_newfromlonglong(testinterp, NULL, -123LL), 1 },
		{ classobject_newinstance(testinterp, NULL, objectclass, NULL, NULL), 1 },
		{ stringobject_newfromcharptr(testinterp, NULL, "asd"), 1 },
		{ arrayobject_newempty(testinterp, NULL), 0 }
	};
	err = NULL;
	OBJECT_DECREF(testinterp, objectclass);

	for (unsigned int i=0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		struct HashTest test = tests[i];
		buttert(test.obj);
		buttert(test.shouldBhashable == !!test.shouldBhashable);
		if (test.shouldBhashable) {
			buttert(test.obj->hashable == 1);

			// just to make sure that the hash is accessed and valgrind complains if it's not set
			// if you're really unlucky, this fails when things actually work
			buttert(test.obj->hash != 123456);
		} else {
			buttert(test.obj->hashable == 0);
			// don't access test.obj.hash here
		}

		OBJECT_DECREF(testinterp, test.obj);
	}
}


// classobject isn't tested here because it's used a lot when setting up testinterp
