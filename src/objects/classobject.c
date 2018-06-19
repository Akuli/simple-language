#include "classobject.h"
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../attribute.h"
#include "../check.h"
#include "../interpreter.h"
#include "../method.h"
#include "../objectsystem.h"
#include "../unicode.h"
#include "array.h"
#include "errors.h"
#include "mapping.h"
#include "null.h"
#include "string.h"

static void class_foreachref(struct Object *klass, void *cbdata, object_foreachrefcb cb)
{
	struct ClassObjectData *data = klass->data;
	if (!data)     // setup method failed
		return;
	if (data->baseclass) cb(data->baseclass, cbdata);
	if (data->setters) cb(data->setters, cbdata);
	if (data->getters) cb(data->getters, cbdata);
}

static void class_destructor(struct Object *klass)
{
	// class_foreachref takes care of most other things
	struct ClassObjectData *data = klass->data;
	if (data) {
		free(data->name.val);    // free(NULL) does nothing
		free(data);
	}
}


static struct ClassObjectData *create_data(struct Interpreter *interp, struct Object *baseclass, bool inheritable)
{
	struct ClassObjectData *data = malloc(sizeof(struct ClassObjectData));
	if (!data)
		return NULL;

	data->name.len = 0;
	data->name.val = NULL;
	data->baseclass = baseclass;
	if (baseclass) {
		OBJECT_INCREF(interp, baseclass);
		data->newinstance = ((struct ClassObjectData *) baseclass->data)->newinstance;   // may be NULL
	} else
		data->newinstance = NULL;
	data->setters = NULL;
	data->getters = NULL;
	data->inheritable = inheritable;
	return data;
}

// only for error handling, class_destructor() handles other cases
static void free_data(struct Interpreter *interp, struct ClassObjectData *data)
{
	if (data->baseclass)
		OBJECT_DECREF(interp, data->baseclass);
	free(data);
}

struct Object *classobject_new_noerr(struct Interpreter *interp, struct Object *baseclass, bool inheritable, functionobject_cfunc newinstance)
{
	struct ClassObjectData *data = create_data(interp, baseclass, inheritable);
	if (!data)
		return NULL;

	if (newinstance)
		data->newinstance = newinstance;
	// else, use the baseclass newinstance, see create_data

	// interp->Class can be NULL, see builtins_setup()
	struct Object *klass = object_new_noerr(interp, interp->builtins.Class, data, class_foreachref, class_destructor);
	if (!klass) {
		free_data(interp, data);
		return NULL;
	}
	return klass;
}

bool classobject_setname(struct Interpreter *interp, struct Object *klass, char *name)
{
	struct ClassObjectData *data = klass->data;

	// if utf8_decode fails, the name must be left untouched
	void *maybegonnafree = data->name.val;
	bool ok = utf8_decode(interp, name, strlen(name), &data->name);
	if (ok)
		free(maybegonnafree);    // free(NULL) does nothing
	return ok;
}

static bool check_inheritable(struct Interpreter *interp, struct Object *baseclass)
{
	struct ClassObjectData *data = baseclass->data;
	if (!data->inheritable) {
		errorobject_throwfmt(interp, "TypeError", "cannot inherit from non-inheritable class %U", data->name);
		return false;
	}
	return true;
}

struct Object *classobject_new(struct Interpreter *interp, char *name, struct Object *baseclass, bool inheritable, functionobject_cfunc newinstance)
{
	assert(interp->builtins.Class);
	assert(interp->builtins.nomemerr);

	if (!check_inheritable(interp, baseclass))
		return NULL;

	struct Object *klass = classobject_new_noerr(interp, baseclass, inheritable, newinstance);
	if (!klass) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	if (!classobject_setname(interp, klass, name)) {
		OBJECT_DECREF(interp, klass);
		return NULL;
	}
	return klass;
}


bool classobject_issubclassof(struct Object *sub, struct Object *super)
{
	do {
		if (sub == super)
			return true;
	} while ((sub = ((struct ClassObjectData*) sub->data)->baseclass));
	return false;
}


static struct Object *class_newinstance(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, interp->builtins.String, interp->builtins.Class, NULL)) return NULL;
	if (!check_opts(interp, opts, "inheritable", interp->builtins.Object, NULL)) return NULL;
	struct Object *classclass = ARRAYOBJECT_GET(args, 0);
	struct Object *name = ARRAYOBJECT_GET(args, 1);
	struct Object *baseclass = ARRAYOBJECT_GET(args, 2);

	if (!check_inheritable(interp, baseclass))
		return NULL;

	// check if the class is supposed to be inheritable
	// TODO: add some utility function! this is awful
	struct Object *inheritableobj;
	struct Object *tmp = stringobject_newfromcharptr(interp, "inheritable");
	if (!tmp)
		return NULL;
	int status = mappingobject_get(interp, opts, tmp, &inheritableobj);
	OBJECT_DECREF(interp, tmp);

	bool inheritable;
	if (status == 0)
		inheritable = false;
	else if (status == -1)
		return NULL;
	else {
		struct Object *yes = interpreter_getbuiltin(interp, "true");
		if (!yes) {
			OBJECT_DECREF(interp, inheritableobj);
			return NULL;
		}
		struct Object *no = interpreter_getbuiltin(interp, "false");
		if (!no) {
			OBJECT_DECREF(interp, yes);
			OBJECT_DECREF(interp, inheritableobj);
			return NULL;
		}

		bool yess = (inheritableobj == yes);
		bool noo = (inheritableobj == no);
		OBJECT_DECREF(interp, yes);
		OBJECT_DECREF(interp, no);
		OBJECT_DECREF(interp, inheritableobj);

		if (!yess && !noo) {
			errorobject_throwfmt(interp, "TypeError", "inheritable must be a Bool, not %D", inheritableobj);
			return NULL;
		}
		inheritable = yess;
	}

	// phewhhh.... check done

	struct ClassObjectData *data = create_data(interp, baseclass, inheritable);
	if (!data) {
		errorobject_thrownomem(interp);
		return NULL;
	}

	assert(!data->name.val);   // shouldn't need a free() here
	if (!unicodestring_copyinto(interp, *((struct UnicodeString *) name->data), &data->name)) {
		free_data(interp, data);
		return NULL;
	}

	struct Object *klass = object_new_noerr(interp, classclass, data, class_foreachref, class_destructor);
	if (!klass) {
		free(data->name.val);
		free_data(interp, data);
		errorobject_thrownomem(interp);
		return NULL;
	}
	return klass;
}

struct Object *classobject_create_Class_noerr(struct Interpreter *interp)
{
	return classobject_new_noerr(interp, interp->builtins.Object, false /* no metaclasses :( */, class_newinstance);
}


// override Object's setup to allow arguments
static struct Object *setup(struct Interpreter *interp, struct Object *args, struct Object *opts) {
	return nullobject_get(interp);
}

static struct Object *name_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	return stringobject_newfromustr(interp, data->name);
}

static struct Object *baseclass_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;

	if (!data->baseclass) {
		// it's Object
		// it could be some other class too, but only Object SHOULD have this....
		return nullobject_get(interp);
	}
	OBJECT_INCREF(interp, data->baseclass);
	return data->baseclass;
}

static struct Object *getters_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	if (!data->getters) {
		if (!(data->getters = mappingobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->getters);
	return data->getters;
}

static struct Object *setters_getter(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;

	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	if (!data->setters) {
		if (!(data->setters = mappingobject_newempty(interp)))
			return NULL;
	}

	OBJECT_INCREF(interp, data->setters);
	return data->setters;
}

static struct Object *to_debug_string(struct Interpreter *interp, struct Object *args, struct Object *opts)
{
	if (!check_args(interp, args, interp->builtins.Class, NULL)) return NULL;
	if (!check_no_opts(interp, opts)) return NULL;
	struct ClassObjectData *data = ARRAYOBJECT_GET(args, 0)->data;
	return stringobject_newfromfmt(interp, "<Class \"%U\">", data->name);
}

bool classobject_addmethods(struct Interpreter *interp)
{
	if (!method_add(interp, interp->builtins.Class, "setup", setup)) return false;
	if (!method_add(interp, interp->builtins.Class, "to_debug_string", to_debug_string)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "name", name_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "baseclass", baseclass_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "getters", getters_getter, NULL)) return false;
	if (!attribute_add(interp, interp->builtins.Class, "setters", setters_getter, NULL)) return false;
	return true;
}
