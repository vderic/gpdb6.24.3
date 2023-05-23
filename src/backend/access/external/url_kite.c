// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "postgres.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "access/fileam.h"
#include "cdb/cdbtimer.h"
#include "cdb/cdbvars.h"
#include "libpq/pqsignal.h"
#include "utils/resowner.h"

#if 0
#include "exx/exx_xdrclnt_wrap.h"
#endif
#include "kitesdk.h"
#include "exx/exx_kite_url.h"


URL_FILE *url_kite_fopen(char *url, bool forwrite, extvar_t *ev, CopyState pstate)
{
	URL_KITE_FILE *file = (URL_KITE_FILE *) palloc(sizeof(URL_KITE_FILE));
	file->common.type = CFTYPE_KITE;
	file->common.url = pstrdup(url);
	file->seq_number = 0;
	file->hdl = 0;

	return (URL_FILE *) file;

}

void url_kite_fclose(URL_FILE *ufile, bool failOnError, const char *relname)
{
	URL_KITE_FILE *file = (URL_KITE_FILE *) ufile;

	if (file->hdl) {
		kite_release(file->hdl);
		file->hdl = 0;
	}

	if (file->common.url) {
		pfree(file->common.url);
	}
	pfree(file); 
}

bool url_kite_feof(URL_FILE *file, int bytesread)
{
	return bytesread == 0;
}

bool url_kite_ferror(URL_FILE *file, int bytesread, char *ebuf, int ebuflen)
{
	return bytesread < 0;
}

size_t url_kite_fread(void *ptr, size_t size, URL_FILE *ufile, CopyState pstate)
{
	Assert(!"KITE READ SHOULD NOT CALL THIS");
	return 0;
}

size_t url_kite_fwrite(void *ptr, size_t size, URL_FILE *file, CopyState pstate)
{
	Assert(!"KITE WRITE SHOULD NOT CALL THIS"); 
	return 0;
}
