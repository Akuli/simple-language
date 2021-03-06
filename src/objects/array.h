#ifndef OBJECTS_ARRAY_H
#define OBJECTS_ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include "../interpreter.h"    // IWYU pragma: keep
#include "../objectsystem.h"   // IWYU pragma: keep

struct ArrayObjectData {
	size_t len;
	size_t nallocated;    // implementation detail
	struct Object **elems;
};

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_createclass(struct Interpreter *interp);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_new(struct Interpreter *interp, struct Object **elems, size_t nelems);

// for convenience, 3 feels good
#define arrayobject_newempty(interp) arrayobject_newwithcapacity((interp), 3)

// like arrayobject_newempty, capacity is the number of items that can be pushed efficiently
struct Object *arrayobject_newwithcapacity(struct Interpreter *interp, size_t capacity);

// bad things happen if arr is not an array object or i < ARRAYOBJECT_LEN(arr)
// otherwise these never fail
// these DO NOT return a new reference!
#define ARRAYOBJECT_GET(arr, i) (((struct ArrayObjectData *) (arr)->objdata.data)->elems[(i)])
#define ARRAYOBJECT_LEN(arr)    (((struct ArrayObjectData *) (arr)->objdata.data)->len)

// returns a new array with elements from arr1 and arr2
// bad things happen if arr1 ir arr2 is not an array
struct Object *arrayobject_concat(struct Interpreter *interp, struct Object *arr1, struct Object *arr2);

// returns false on error
// bad things happen if arr is not an array object
bool arrayobject_push(struct Interpreter *interp, struct Object *arr, struct Object *elem);

// RETURNS A NEW REFERENCE, or NULL if the array is empty
// never fails if arr is an array object, bad things happen if it isn't
struct Object *arrayobject_pop(struct Interpreter *interp, struct Object *arr);

// RETURNS A NEW REFERENCE or NULL on error
struct Object *arrayobject_slice(struct Interpreter *interp, struct Object *arr, long long start, long long end);

#endif    // OBJECTS_ARRAY_H
