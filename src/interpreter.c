#include "interpreter.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>       // this thing prints messages for debugging refcount problems
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "context.h"
#include "hashtable.h"
#include "unicode.h"

static int compare_by_identity(void *a, void *b, void *junkdata) { return a==b; }

struct Interpreter *interpreter_new(char *argv0)
{
	struct Interpreter *interp = malloc(sizeof(struct Interpreter));
	if (!interp)
		return NULL;

	interp->allobjects = hashtable_new(compare_by_identity);
	if (!(interp->allobjects)) {
		free(interp);
		return NULL;
	}

	interp->builtinctx = context_newglobal(interp);
	if (!(interp->builtinctx)) {
		hashtable_free(interp->allobjects);
		free(interp);
		return NULL;
	}

	interp->argv0 = argv0;
	interp->nomemerr = NULL;
	interp->classclass = NULL;
	interp->functionclass = NULL;
	return interp;
}

void interpreter_free(struct Interpreter *interp)
{
	/* interp->builtinctx is freed in builtins_teardown() because it also frees
	   stuff that are needed for freeing interp->builtinctx */

	hashtable_free(interp->allobjects);   // fails if there are any objects left
	free(interp);
}

int interpreter_addbuiltin(struct Interpreter *interp, struct Object **errptr, char *name, struct Object *val)
{
	struct UnicodeString uname;
	char stupiderrormsg[100] = {0};
	int status = utf8_decode(name, strlen(name), &uname, stupiderrormsg);
	if (status == STATUS_NOMEM) {
		*errptr = interp->nomemerr;
		return STATUS_ERROR;
	}
	assert(status == STATUS_OK && stupiderrormsg[0] == 0);  // must be valid utf8

	int res = context_setlocalvar(interp->builtinctx, errptr, uname, val);
	free(uname.val);
	return res;
}

struct Object *interpreter_getbuiltin(struct Interpreter *interp, struct Object **errptr, char *name)
{
	struct UnicodeString uname;
	char stupiderrormsg[100] = {0};
	int status = utf8_decode(name, strlen(name), &uname, stupiderrormsg);
	if (status == STATUS_NOMEM) {
		*errptr = interp->nomemerr;
		return NULL;
	}
	assert(status == STATUS_OK && stupiderrormsg[0] == 0);  // must be valid utf8

	struct Object *res = context_getvar(interp->builtinctx, errptr, uname);
	free(uname.val);
	return res;
}

struct Object *interpreter_getbuiltin_nomalloc(struct Interpreter *interp, char *name)
{
	size_t namelen = strlen(name);
	assert(namelen < 50);

	uint32_t unameval[50];
	for (int i=0; i < (int)namelen; i++)
		unameval[i] = name[i];
	struct UnicodeString uname = { unameval, namelen };

	return context_getvar_nomalloc(interp->builtinctx, uname);
}
