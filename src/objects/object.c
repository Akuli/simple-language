#include "object.h"
#include <stddef.h>
#include "array.h"
#include "classobject.h"
#include "errors.h"
#include "function.h"
#include "integer.h"
#include "string.h"
#include "../common.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"

static void object_foreachref(struct Object *obj, void *data, classobject_foreachrefcb cb)
{
	if (obj->klass)
		cb(obj->klass, data);
	if (obj->attrs)
		cb(obj->attrs, data);
}

struct Object *objectobject_createclass_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, "Object", NULL, 0, object_foreachref);
}


static struct Object *to_string(struct Interpreter *interp, struct Object *argarr)
{
	// functionobject_checktypes may call to_string when creating an error message
	// so we can't use it here, otherwise this may recurse
	// maybe it wouldn't recurse... but better safe than sorry
	if (ARRAYOBJECT_LEN(argarr) != 1) {
		errorobject_setwithfmt(interp, "Object::to_string takes exactly 1 argument");
		return NULL;
	}

	char *name = ((struct ClassObjectData*) ARRAYOBJECT_GET(argarr, 0)->klass->data)->name;
	return stringobject_newfromfmt(interp, "<%s at %p>", name, (void *) ARRAYOBJECT_GET(argarr, 0));
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *argarr)
{
	if (ARRAYOBJECT_LEN(argarr) != 1) {
		errorobject_setwithfmt(interp, "Object::to_debug_string takes exactly 1 argument");
		return NULL;
	}
	return method_call(interp, ARRAYOBJECT_GET(argarr, 0), "to_string", NULL);
}

int objectobject_addmethods(struct Interpreter *interp)
{
	if (method_add(interp, interp->builtins.objectclass, "to_string", to_string) == STATUS_ERROR) return STATUS_ERROR;
	if (method_add(interp, interp->builtins.objectclass, "to_debug_string", to_debug_string) == STATUS_ERROR) return STATUS_ERROR;
	return STATUS_OK;
}
