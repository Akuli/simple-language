#include "run.h"
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include "ast.h"
#include "common.h"
#include "context.h"
#include "interpreter.h"
#include "method.h"
#include "objectsystem.h"
#include "objects/array.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/function.h"
#include "objects/integer.h"
#include "objects/string.h"

// RETURNS A NEW REFERENCE
static struct Object *run_expression(struct Context *ctx, struct Object **errptr, struct AstNode *expr)
{
#define INFO_AS(X) ((struct X *)expr->info)
	if (expr->kind == AST_GETVAR)
		return context_getvar(ctx, errptr, INFO_AS(AstGetVarInfo)->varname);
	if (expr->kind == AST_STR)
		return stringobject_newfromustr(ctx->interp, errptr, *((struct UnicodeString *)expr->info));
	if (expr->kind == AST_INT)
		return integerobject_newfromcharptr(ctx->interp, errptr, INFO_AS(AstIntInfo)->valstr);

	if (expr->kind == AST_GETMETHOD) {
		struct Object *obj = run_expression(ctx, errptr, INFO_AS(AstGetAttrOrMethodInfo)->obj);
		if (!obj)
			return NULL;

		struct Object *res = method_getwithustr(ctx->interp, errptr, obj, INFO_AS(AstGetAttrOrMethodInfo)->name);
		OBJECT_DECREF(ctx->interp, obj);
		return res;
	}

	if (expr->kind == AST_CALL) {
		struct Object *func = run_expression(ctx, errptr, INFO_AS(AstCallInfo)->func);
		if (!func)
			return NULL;

		// TODO: better type check
		assert(func->klass == ctx->interp->functionclass);

		struct Object **args = malloc(sizeof(struct Object*) * INFO_AS(AstCallInfo)->nargs);
		if (!args) {
			OBJECT_DECREF(ctx->interp, func);
			errorobject_setnomem(ctx->interp, errptr);
			return NULL;
		}

		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++) {
			struct Object *arg = run_expression(ctx, errptr, INFO_AS(AstCallInfo)->args[i]);
			if (!arg) {
				for (size_t j=0; j<i; j++)
					OBJECT_DECREF(ctx->interp, args[j]);
				free(args);
				OBJECT_DECREF(ctx->interp, func);
				return NULL;
			}
			args[i] = arg;
		}

		struct Object *res = functionobject_vcall(ctx, errptr, func, args, INFO_AS(AstCallInfo)->nargs);
		for (size_t i=0; i < INFO_AS(AstCallInfo)->nargs; i++)
			OBJECT_DECREF(ctx->interp, args[i]);
		free(args);
		OBJECT_DECREF(ctx->interp, func);
		return res;
	}

	if (expr->kind == AST_ARRAY) {
		// this is handled specially because malloc(0) MAY return NULL
		if (INFO_AS(AstArrayOrBlockInfo)->nitems == 0)
			return arrayobject_newempty(ctx->interp, errptr);

		// TODO: create a DynamicArray directly instead of a temporary shit
		struct Object **elems = malloc(sizeof(struct Object *) * INFO_AS(AstArrayOrBlockInfo)->nitems);
		if (!elems) {
			errorobject_setnomem(ctx->interp, errptr);
			return NULL;
		}

		for (size_t i=0; i < INFO_AS(AstArrayOrBlockInfo)->nitems; i++) {
			elems[i] = run_expression(ctx, errptr, INFO_AS(AstArrayOrBlockInfo)->items[i]);
			if (!elems[i]) {
				for (size_t j=0; j<i; j++)
					OBJECT_DECREF(ctx->interp, elems[i]);
				free(elems);
				return NULL;
			}
		}

		struct Object *arr = arrayobject_new(ctx->interp, errptr, elems, INFO_AS(AstArrayOrBlockInfo)->nitems);
		for (size_t i=0; i < INFO_AS(AstArrayOrBlockInfo)->nitems; i++)
			OBJECT_DECREF(ctx->interp, elems[i]);
		free(elems);
		return arr;
	}
#undef INFO_AS

	assert(0);
}

int run_statement(struct Context *ctx, struct Object **errptr, struct AstNode *stmt)
{
#define INFO_AS(X) ((struct X *)stmt->info)
	if (stmt->kind == AST_CALL) {
		struct Object *ret = run_expression(ctx, errptr, stmt);
		if (!ret)
			return STATUS_ERROR;

		OBJECT_DECREF(ctx->interp, ret);  // ignore the return value
		return STATUS_OK;
	}

	if (stmt->kind == AST_CREATEVAR) {
		struct Object *val = run_expression(ctx, errptr, INFO_AS(AstCreateOrSetVarInfo)->val);
		if (!val)
			return STATUS_ERROR;

		int status = context_setlocalvar(ctx, errptr, INFO_AS(AstCreateOrSetVarInfo)->varname, val);
		OBJECT_DECREF(ctx->interp, val);
		return status;
	}
#undef INFO_AS

	assert(0);
}
