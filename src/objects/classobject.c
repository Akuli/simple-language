#include "classobject.h"
#include <assert.h>
#include <stdlib.h>
#include "../common.h"
#include "../interpreter.h"
#include "../objectsystem.h"

static void classobject_free(struct Object *obj)
{
	// TODO: objectclassinfo_free(obj->data);  ???
}

int classobject_createclass(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *objectclass)
{
	struct ObjectClassInfo *res = objectclassinfo_new(objectclass, NULL, classobject_free);
	if (!res) {
		*errptr = interp->nomemerr;
		// TODO: decref objectclass or something?
		return STATUS_ERROR;
	}
	interp->classobjectinfo = res;
	return STATUS_OK;
}

struct Object *classobject_new(struct Interpreter *interp, struct Object **errptr, struct Object *base, objectclassinfo_foreachref foreachref, void (*destructor)(struct Object *))
{
	// TODO: better type check
	assert(base->klass == interp->classobjectinfo);

	struct ObjectClassInfo *info = objectclassinfo_new((struct ObjectClassInfo*) base->data, foreachref, destructor);
	if (!info) {
		*errptr = interp->nomemerr;
		return NULL;
	}

	struct Object *klass = classobject_newfromclassinfo(interp, errptr, info);
	if (!klass) {
		objectclassinfo_free(info);
		return NULL;
	}

	return klass;
}

struct Object *classobject_newinstance(struct Interpreter *interp, struct Object **errptr, struct Object *klass, void *data)
{
	assert(klass->klass == interp->classobjectinfo);      // TODO: better type check
	struct Object *res = object_new(interp, klass->data, data);
	if (!res) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	return res;
}

struct Object *classobject_newfromclassinfo(struct Interpreter *interp, struct Object **errptr, struct ObjectClassInfo *wrapped)
{
	assert(interp->classobjectinfo);
	struct Object *klass = object_new(interp, interp->classobjectinfo, wrapped);
	if (!klass) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	return klass;
}
