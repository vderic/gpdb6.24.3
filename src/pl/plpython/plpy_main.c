/*
 * PL/Python main entry points
 *
 * src/pl/plpython/plpy_main.c
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/trigger.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/syscache.h"

#include "cdb/cdbvars.h"

#include "plpython.h"

#include "plpy_main.h"

#include "plpy_elog.h"
#include "plpy_exec.h"
#include "plpy_plpymodule.h"
#include "plpy_procedure.h"
#include "plpy_subxactobject.h"


/*
 * exported functions
 */

#if PY_MAJOR_VERSION >= 3
/* Use separate names to avoid clash in pg_pltemplate */
#define plpython_validator plpython3_validator
#define plpython_call_handler plpython3_call_handler
#define plpython_inline_handler plpython3_inline_handler
#endif

extern void _PG_init(void);

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(plpython_validator);
PG_FUNCTION_INFO_V1(plpython_call_handler);
PG_FUNCTION_INFO_V1(plpython_inline_handler);

#if PY_MAJOR_VERSION < 3
/* Define aliases plpython2_call_handler etc */
PG_FUNCTION_INFO_V1(plpython2_validator);
PG_FUNCTION_INFO_V1(plpython2_call_handler);
PG_FUNCTION_INFO_V1(plpython2_inline_handler);
#endif


static bool PLy_procedure_is_trigger(Form_pg_proc procStruct);
static void plpython_error_callback(void *arg);
static void plpython_inline_error_callback(void *arg);
static void PLy_init_interp(void);

static PLyExecutionContext *PLy_push_execution_context(void);
static void PLy_pop_execution_context(void);

/* static state for Python library conflict detection */
static int *plpython_version_bitmask_ptr = NULL;
static int	plpython_version_bitmask = 0;
static const int plpython_python_version = PY_MAJOR_VERSION;

/* initialize global variables */
PyObject   *PLy_interp_globals = NULL;

/* this doesn't need to be global; use PLy_current_execution_context() */
static PLyExecutionContext *PLy_execution_contexts = NULL;

/* For GPDB Use: Query cancel supported */
cancel_pending_hook_type prev_cancel_pending_hook;

void PLy_handle_cancel_interrupt(void);

bool PLy_enter_python_intepreter = false;

static bool inited = false;

/* GUC variables */
#if PY_MAJOR_VERSION >= 3
static char *plpython3_path = NULL;

static bool
plpython3_check_python_path(char **newval, void **extra, GucSource source) {
	if (inited)
	{
		GUC_check_errmsg("SET PYTHONPATH failed, the GUC value can only be changed before initializing the python interpreter.");
		return false;
	}
	return true;
}
#endif

void
_PG_init(void)
{
	int		  **bitmask_ptr;
	const int **version_ptr;

	/*
	 * Set up a shared bitmask variable telling which Python version(s) are
	 * loaded into this process's address space.  If there's more than one, we
	 * cannot call into libpython for fear of causing crashes.  But postpone
	 * the actual failure for later, so that operations like pg_restore can
	 * load more than one plpython library so long as they don't try to do
	 * anything much with the language.
	 */
	bitmask_ptr = (int **) find_rendezvous_variable("plpython_version_bitmask");
	if (!(*bitmask_ptr))		/* am I the first? */
		*bitmask_ptr = &plpython_version_bitmask;
	/* Retain pointer to the agreed-on shared variable ... */
	plpython_version_bitmask_ptr = *bitmask_ptr;
	/* ... and announce my presence */
	*plpython_version_bitmask_ptr |= (1 << PY_MAJOR_VERSION);

	/*
	 * This should be safe even in the presence of conflicting plpythons, and
	 * it's necessary to do it here for the next error to be localized.
	 */
	pg_bindtextdomain(TEXTDOMAIN);

	/*
	 * We used to have a scheme whereby PL/Python would fail immediately if
	 * loaded into a session in which a conflicting libpython is already
	 * present.  We don't like to do that anymore, but it seems possible that
	 * a plpython library adhering to the old convention is present in the
	 * session, in which case we have to fail.  We detect an old library if
	 * plpython_python_version is already defined but the indicated version
	 * isn't reflected in plpython_version_bitmask.  Otherwise, set the
	 * variable so that the right thing happens if an old library is loaded
	 * later.
	 */
	version_ptr = (const int **) find_rendezvous_variable("plpython_python_version");
	if (!(*version_ptr))
		*version_ptr = &plpython_python_version;
	else
	{
		if ((*plpython_version_bitmask_ptr & (1 << **version_ptr)) == 0)
			ereport(FATAL,
					(errmsg("Python major version mismatch in session"),
					 errdetail("This session has previously used Python major version %d, and it is now attempting to use Python major version %d.",
							   **version_ptr, plpython_python_version),
					 errhint("Start a new session to use a different Python major version.")));
	}

	/* Register SIGINT/SIGTERM handler for python */
	prev_cancel_pending_hook = cancel_pending_hook;
	cancel_pending_hook = PLy_handle_cancel_interrupt;

#if PY_MAJOR_VERSION >= 3
	DefineCustomStringVariable("plpython3.python_path",
							gettext_noop("PYTHONPATH for plpython3."),
							NULL,
							&plpython3_path,
							"", // default path need to set empty for init
							PGC_USERSET,
							GUC_GPDB_NEED_SYNC,
							plpython3_check_python_path,
							NULL,
							NULL);
#endif
	pg_bindtextdomain(TEXTDOMAIN);
}


/*
 * Perform one-time setup of PL/Python, after checking for a conflict
 * with other versions of Python.
 */
static void
PLy_initialize(void)
{

	/*
	 * Check for multiple Python libraries before actively doing anything with
	 * libpython.  This must be repeated on each entry to PL/Python, in case a
	 * conflicting library got loaded since we last looked.
	 *
	 * It is attractive to weaken this error from FATAL to ERROR, but there
	 * would be corner cases, so it seems best to be conservative.
	 */
	if (*plpython_version_bitmask_ptr != (1 << PY_MAJOR_VERSION))
		ereport(FATAL,
				(errmsg("multiple Python libraries are present in session"),
				 errdetail("Only one Python major version can be used in one session.")));
#if PY_MAJOR_VERSION >= 3
	/* PYTHONPATH and PYTHONHOME has been set to GPDB's python2.7 in Postmaster when
	 * gpstart. So for plpython3u, we need to unset PYTHONPATH and PYTHONHOME.
	 * if user set PYTHONPATH then we set it in the env
	 */
	if (plpython3_path && *plpython3_path)
	{
		setenv("PYTHONPATH", plpython3_path, 1);
	}
	else
	{
		unsetenv("PYTHONPATH");
	}
	unsetenv("PYTHONHOME");
#endif
	/* The rest should only be done once per session */
	if (inited)
		return;

#if PY_MAJOR_VERSION >= 3
	PyImport_AppendInittab("plpy", PyInit_plpy);
#endif
	Py_Initialize();
#if PY_MAJOR_VERSION >= 3
	PyImport_ImportModule("plpy");
#endif
	PLy_init_interp();
	PLy_init_plpy();
	if (PyErr_Occurred())
		PLy_elog(FATAL, "untrapped error in initialization");

	init_procedure_caches();

	explicit_subtransactions = NIL;

	PLy_execution_contexts = NULL;

	inited = true;
}

/*
 * For GPDB Use:
 * Raise a KeyboardInterrupt exception, to simulate a SIGINT.
 */
int
PLy_python_cancel_handler(void *arg)
{
	PyErr_SetNone(PyExc_KeyboardInterrupt);

	/* return -1 to indicate that we set an exception. */
	return -1;
}

/*
 * For GPDB Use: Hook function, called when current query is being cancelled
 * (on e.g. SIGINT or SIGTERM)
 *
 * NB: This is called from a signal handler!
 */
void
PLy_handle_cancel_interrupt(void)
{
	/*
	 * We can't do much in a signal handler, so just tell the Python
	 * interpreter to call us back when possible.
	 *
	 * We don't bother to check the return value, as there's nothing we could
	 * do if it fails for some reason.
	 */
	if (PLy_enter_python_intepreter)
		(void) Py_AddPendingCall(PLy_python_cancel_handler, NULL);

	if (prev_cancel_pending_hook)
		prev_cancel_pending_hook();
}

/*
 * This should be called only once, from PLy_initialize. Initialize the Python
 * interpreter and global data.
 */
static void
PLy_init_interp(void)
{
	static PyObject *PLy_interp_safe_globals = NULL;
	PyObject   *mainmod;

	mainmod = PyImport_AddModule("__main__");
	if (mainmod == NULL || PyErr_Occurred())
		PLy_elog(ERROR, "could not import \"__main__\" module");
	Py_INCREF(mainmod);
	PLy_interp_globals = PyModule_GetDict(mainmod);
	PLy_interp_safe_globals = PyDict_New();
	if (PLy_interp_safe_globals == NULL)
		PLy_elog(ERROR, "could not create globals");
	PyDict_SetItemString(PLy_interp_globals, "GD", PLy_interp_safe_globals);
	Py_DECREF(mainmod);
	if (PLy_interp_globals == NULL || PyErr_Occurred())
		PLy_elog(ERROR, "could not initialize globals");
}

Datum
plpython_validator(PG_FUNCTION_ARGS)
{
	Oid			funcoid = PG_GETARG_OID(0);
	HeapTuple	tuple;
	Form_pg_proc procStruct;
	bool		is_trigger;

	if (!CheckFunctionValidatorAccess(fcinfo->flinfo->fn_oid, funcoid))
		PG_RETURN_VOID();

	if (!check_function_bodies)
		PG_RETURN_VOID();

	/* Do this only after making sure we need to do something */
	PLy_initialize();

	/* Get the new function's pg_proc entry */
	tuple = SearchSysCache1(PROCOID, ObjectIdGetDatum(funcoid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for function %u", funcoid);
	procStruct = (Form_pg_proc) GETSTRUCT(tuple);

	is_trigger = PLy_procedure_is_trigger(procStruct);

	ReleaseSysCache(tuple);

	/* We can't validate triggers against any particular table ... */
	PLy_procedure_get(funcoid, InvalidOid, is_trigger);

	PG_RETURN_VOID();
}

#if PY_MAJOR_VERSION < 3
Datum
plpython2_validator(PG_FUNCTION_ARGS)
{
	/* call plpython validator with our fcinfo so it gets our oid */
	return plpython_validator(fcinfo);
}
#endif   /* PY_MAJOR_VERSION < 3 */

Datum
plpython_call_handler(PG_FUNCTION_ARGS)
{
	Datum		retval;
	PLyExecutionContext *exec_ctx;
	ErrorContextCallback plerrcontext;

	PLy_initialize();

	/* Note: SPI_finish() happens in plpy_exec.c, which is dubious design */
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	/*
	 * Push execution context onto stack.  It is important that this get
	 * popped again, so avoid putting anything that could throw error between
	 * here and the PG_TRY.
	 */
	exec_ctx = PLy_push_execution_context();

	PG_TRY();
	{
		Oid			funcoid = fcinfo->flinfo->fn_oid;
		PLyProcedure *proc;

		/*
		 * Setup error traceback support for ereport().  Note that the PG_TRY
		 * structure pops this for us again at exit, so we needn't do that
		 * explicitly, nor do we risk the callback getting called after we've
		 * destroyed the exec_ctx.
		 */
		plerrcontext.callback = plpython_error_callback;
		plerrcontext.arg = exec_ctx;
		plerrcontext.previous = error_context_stack;
		error_context_stack = &plerrcontext;

		if (CALLED_AS_TRIGGER(fcinfo))
		{
			Relation	tgrel = ((TriggerData *) fcinfo->context)->tg_relation;
			HeapTuple	trv;

			proc = PLy_procedure_get(funcoid, RelationGetRelid(tgrel), true);
			exec_ctx->curr_proc = proc;
			trv = PLy_exec_trigger(fcinfo, proc);
			retval = PointerGetDatum(trv);
		}
		else
		{
			proc = PLy_procedure_get(funcoid, InvalidOid, false);
			exec_ctx->curr_proc = proc;
			retval = PLy_exec_function(fcinfo, proc);
		}
	}
	PG_CATCH();
	{
		PLy_pop_execution_context();
		PyErr_Clear();
		PG_RE_THROW();
	}
	PG_END_TRY();

	/* Destroy the execution context */
	PLy_pop_execution_context();

	return retval;
}

#if PY_MAJOR_VERSION < 3
Datum
plpython2_call_handler(PG_FUNCTION_ARGS)
{
	return plpython_call_handler(fcinfo);
}
#endif   /* PY_MAJOR_VERSION < 3 */

Datum
plpython_inline_handler(PG_FUNCTION_ARGS)
{
	InlineCodeBlock *codeblock = (InlineCodeBlock *) DatumGetPointer(PG_GETARG_DATUM(0));
	FunctionCallInfoData fake_fcinfo;
	FmgrInfo	flinfo;
	PLyProcedure proc;
	PLyExecutionContext *exec_ctx;
	ErrorContextCallback plerrcontext;

	PLy_initialize();

	/* Note: SPI_finish() happens in plpy_exec.c, which is dubious design */
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	MemSet(&fake_fcinfo, 0, sizeof(fake_fcinfo));
	MemSet(&flinfo, 0, sizeof(flinfo));
	fake_fcinfo.flinfo = &flinfo;
	flinfo.fn_oid = InvalidOid;
	flinfo.fn_mcxt = CurrentMemoryContext;

	MemSet(&proc, 0, sizeof(PLyProcedure));
	proc.pyname = PLy_strdup("__plpython_inline_block");
	proc.result.out.d.typoid = VOIDOID;

	/*
	 * Push execution context onto stack.  It is important that this get
	 * popped again, so avoid putting anything that could throw error between
	 * here and the PG_TRY.
	 */
	exec_ctx = PLy_push_execution_context();

	PG_TRY();
	{
		/*
		 * Setup error traceback support for ereport().
		 * plpython_inline_error_callback doesn't currently need exec_ctx, but
		 * for consistency with plpython_call_handler we do it the same way.
		 */
		plerrcontext.callback = plpython_inline_error_callback;
		plerrcontext.arg = exec_ctx;
		plerrcontext.previous = error_context_stack;
		error_context_stack = &plerrcontext;

		PLy_procedure_compile(&proc, codeblock->source_text);
		exec_ctx->curr_proc = &proc;
		PLy_exec_function(&fake_fcinfo, &proc);
	}
	PG_CATCH();
	{
		PLy_pop_execution_context();
		PLy_procedure_delete(&proc);
		PyErr_Clear();
		PG_RE_THROW();
	}
	PG_END_TRY();

	/* Destroy the execution context */
	PLy_pop_execution_context();

	/* Now clean up the transient procedure we made */
	PLy_procedure_delete(&proc);

	PG_RETURN_VOID();
}

#if PY_MAJOR_VERSION < 3
Datum
plpython2_inline_handler(PG_FUNCTION_ARGS)
{
	return plpython_inline_handler(fcinfo);
}
#endif   /* PY_MAJOR_VERSION < 3 */

static bool
PLy_procedure_is_trigger(Form_pg_proc procStruct)
{
	return (procStruct->prorettype == TRIGGEROID ||
			(procStruct->prorettype == OPAQUEOID &&
			 procStruct->pronargs == 0));
}

static void
plpython_error_callback(void *arg)
{
	PLyExecutionContext *exec_ctx = (PLyExecutionContext *) arg;

	if (exec_ctx->curr_proc)
		errcontext("PL/Python function \"%s\"",
				   PLy_procedure_name(exec_ctx->curr_proc));
}

static void
plpython_inline_error_callback(void *arg)
{
	errcontext("PL/Python anonymous code block");
}

PLyExecutionContext *
PLy_current_execution_context(void)
{
	if (PLy_execution_contexts == NULL)
		elog(ERROR, "no Python function is currently executing");

	return PLy_execution_contexts;
}

static PLyExecutionContext *
PLy_push_execution_context(void)
{
	PLyExecutionContext *context = PLy_malloc(sizeof(PLyExecutionContext));

	context->curr_proc = NULL;
	context->scratch_ctx = AllocSetContextCreate(TopTransactionContext,
												 "PL/Python scratch context",
												 ALLOCSET_DEFAULT_MINSIZE,
												 ALLOCSET_DEFAULT_INITSIZE,
												 ALLOCSET_DEFAULT_MAXSIZE);
	context->next = PLy_execution_contexts;
	PLy_execution_contexts = context;
	return context;
}

static void
PLy_pop_execution_context(void)
{
	PLyExecutionContext *context = PLy_execution_contexts;

	if (context == NULL)
		elog(ERROR, "no Python function is currently executing");

	PLy_execution_contexts = context->next;

	MemoryContextDelete(context->scratch_ctx);
	PLy_free(context);
}
