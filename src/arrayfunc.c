#include "arrayfunc.h"
#include <assert.h>
#include "attribute.h"
#include "common.h"
#include "method.h"
#include "objects/array.h"
#include "objects/function.h"
#include "objects/mapping.h"
#include "objects/scope.h"
#include "objects/string.h"

// this is partialled to a block of code to create array_funcs
static struct Object *runner(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	struct Object *parentscope = attribute_get(interp, args[0], "definition_scope");
	if (!parentscope)
		return NULL;

	struct Object *scope = scopeobject_newsub(interp, parentscope);
	OBJECT_DECREF(interp, parentscope);
	if (!scope)
		return NULL;

	struct Object *localvars = NULL, *string = NULL, *array = NULL;
	if (!((localvars = attribute_get(interp, scope, "local_vars")) &&
			(string = stringobject_newfromcharptr(interp, "arguments")) &&
			(array = arrayobject_new(interp, args+1, nargs-1)))) {
		if (localvars) OBJECT_DECREF(interp, localvars);
		if (string) OBJECT_DECREF(interp, string);
		if (array) OBJECT_DECREF(interp, array);
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	int status = mappingobject_set(interp, localvars, string, array);
	OBJECT_DECREF(interp, localvars);
	OBJECT_DECREF(interp, string);
	OBJECT_DECREF(interp, array);
	if (status == STATUS_ERROR) {
		OBJECT_DECREF(interp, scope);
		return NULL;
	}

	struct Object *res = method_call(interp, args[0], "run", scope, NULL);
	OBJECT_DECREF(interp, scope);
	return res;
}

static struct Object *arrayfunc_builtin(struct Interpreter *interp, struct Object **args, size_t nargs)
{
	if (functionobject_checktypes(interp, args, nargs, interp->builtins.blockclass, NULL) == STATUS_ERROR)
		return NULL;
	struct Object *block = args[0];

	struct Object *runnerobj = functionobject_new(interp, runner);
	if (!runnerobj)
		return NULL;

	struct Object *res = functionobject_newpartial(interp, runnerobj, block);
	OBJECT_DECREF(interp, runnerobj);
	return res;   // may be NULL
}

struct Object *arrayfunc_create(struct Interpreter *interp)
{
	return functionobject_new(interp, arrayfunc_builtin);
}
