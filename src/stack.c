#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "stack.h"

#include "interpreter.h"
#include "objectsystem.h"
#include "objects/errors.h"


bool stack_push(struct Interpreter *interp, char *filename, size_t lineno, struct Object *scope)
{
	if (interp->stackptr - interp->stack == STACK_MAX) {
		// errorobject_throwfmt must not push more frames to the stack!
		// FIXME: ValueError is very wrong for this
		errorobject_throwfmt(interp, "ValueError", "too much recursion");
		return false;
	}

	assert(lineno != 0);

	size_t len = strlen(filename);
	char *tmp = malloc(len+1);
	if (!tmp) {
		errorobject_thrownomem(interp);
		return NULL;
	}
	memcpy(tmp, filename, len+1);

	struct StackFrame f = { .filename = tmp, .lineno = lineno, .scope = scope };
	if (scope)
		OBJECT_INCREF(interp, scope);
	*interp->stackptr++ = f;
	return true;
}

void stack_pop(struct Interpreter *interp)
{
	assert(interp->stack != interp->stackptr);   // stack must not be empty
	struct StackFrame f = *--interp->stackptr;
	free(f.filename);
	if (f.scope)
		OBJECT_DECREF(interp, f.scope);
}
