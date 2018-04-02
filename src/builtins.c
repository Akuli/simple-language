#include "builtins.h"
#include <assert.h>
#include "common.h"
#include "interpreter.h"
#include "objectsystem.h"
#include "objects/classobject.h"
#include "objects/errors.h"
#include "objects/object.h"
#include "objects/string.h"


// this is ugly
int builtins_setup(struct Interpreter *interp, struct Object **errptr)
{
	int status = STATUS_ERROR;   // only used on error
	struct ObjectClassInfo *objectinfo = NULL, *errorinfo = NULL, *stringinfo = NULL;
	struct Object *objectclass = NULL, *errorclass = NULL, *stringclass = NULL;

	if (!(objectinfo = objectobject_createclass())) goto nomem;
	if (!(errorinfo = errorobject_createclass(objectinfo))) goto nomem;
	if (!(stringinfo = stringobject_createclass(objectinfo))) goto nomem;
	if (!(interp->nomemerr = errorobject_createnomemerr(errorinfo, stringinfo))) goto nomem;
	// now we can use errptr

	if (classobject_createclass(interp, errptr, objectinfo) == STATUS_ERROR) goto error;

	if (!(objectclass = classobject_newfromclassinfo(interp, errptr, objectinfo))) goto error;
	if (!(errorclass = classobject_newfromclassinfo(interp, errptr, errorinfo))) goto error;
	if (!(stringclass = classobject_newfromclassinfo(interp, errptr, stringinfo))) goto error;

	if (interpreter_addbuiltin(interp, errptr, "Object", objectclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, errptr, "Error", errorclass) == STATUS_ERROR) goto error;
	if (interpreter_addbuiltin(interp, errptr, "String", stringclass) == STATUS_ERROR) goto error;

	return STATUS_OK;

nomem:
	status = STATUS_NOMEM;
	// "fall through" to error

error:
	// these undo the above stuff, in a reversed order
	// e.g. objectinfo is created first, so it's freed last
	if (stringclass) object_free(stringclass);
	if (errorclass) object_free(errorclass);
	if (objectclass) object_free(objectclass);

	if (interp->classobjectinfo) objectclassinfo_free(interp->classobjectinfo);

	if (interp->nomemerr) {
		object_free(interp->nomemerr->data);   // the message string
		object_free(interp->nomemerr);
	}
	if (stringinfo) objectclassinfo_free(stringinfo);
	if (errorinfo) objectclassinfo_free(stringinfo);
	if (objectinfo) objectclassinfo_free(objectinfo);

	assert(status == STATUS_OK || status == STATUS_NOMEM);
	return status;
}


// TODO: is this too copy/pasta?
void builtins_teardown(struct Interpreter *interp)
{
	// usually lots of other stuff has been freed when this is called, so no mem
	// errors are very unlikely but possible here
	// that's why i think it's ok to ignore any errors are ignored
	struct Object *err;
	struct Object *objectclass = interpreter_getbuiltin(interp, &err, "Object"); err = NULL;
	struct Object *errorclass = interpreter_getbuiltin(interp, &err, "Error"); err = NULL;
	struct Object *stringclass = interpreter_getbuiltin(interp, &err, "String"); err = NULL;
	struct ObjectClassInfo *objectinfo = objectclass ? objectclass->data : NULL;
	struct ObjectClassInfo *errorinfo = errorclass ? errorclass->data : NULL;
	struct ObjectClassInfo *stringinfo = stringclass ? stringclass->data : NULL;

	if (stringclass) object_free(stringclass);
	if (errorclass) object_free(errorclass);
	if (objectclass) object_free(objectclass);

	objectclassinfo_free(interp->classobjectinfo);

	object_free(interp->nomemerr->data);   // the message string
	object_free(interp->nomemerr);
	if (stringinfo) objectclassinfo_free(stringinfo);
	if (errorinfo) objectclassinfo_free(errorinfo);
	if (objectinfo) objectclassinfo_free(objectinfo);

	// TODO: try freeing the errored ones again or something?
}
