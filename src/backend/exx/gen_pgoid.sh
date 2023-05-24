# Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
cat <<HEADER
#ifndef EXX_PGOID_GEN_HPP
#define EXX_PGOID_GEN_HPP

/*
 * DO NOT EDIT.
 *
 * This file is created from pg by gen_pgoid.sh.  
 */
HEADER

echo "/*"
psql template1 -t -c "select version();"
echo "*/"
echo

psql template1 -t -c "select 'const int PG_OPER_' || oprcode || case cnt when 1 then '' else '_' || oid end || ' = ' || oid || ';' from (select oid, oprcode, count(oprcode) over (partition by oprcode) cnt from pg_operator) tmpt order by '' || oprcode"

cat <<OPERSTR_HDR
static inline const char *exx_pgoid_oper_str(int oid) {
	switch(oid) {
OPERSTR_HDR

psql template1 -t -c "select 'case PG_OPER_' || oprcode || case cnt when 1 then '' else '_' || oid end || ': return \"PG_OPER_' || oprcode || case cnt when 1 then '' else '_' || oid end || '\";' from (select oid, oprcode, count(oprcode) over (partition by oprcode) cnt from pg_operator) tmpt order by '' || oprcode"

cat <<OPERSTR_FOOT
	}
	return "unknown oper";
}

OPERSTR_FOOT


echo
echo
echo

psql template1 -t -c "select 'const int PG_PROC_' || proname || case cnt when 1 then '' else '_' || oid end || ' = ' || oid || ';' from (select oid, proname, count(proname) over (partition by proname) cnt from pg_proc) tmpt order by '' || proname"

cat <<PROCSTR_HDR
static inline const char *exx_pgoid_proc_str(int oid) {
	switch(oid) {
PROCSTR_HDR

psql template1 -t -c "select 'case PG_PROC_' || proname || case cnt when 1 then '' else '_' || oid end || ': return \"PG_PROC_' || proname || case cnt when 1 then '' else '_' || oid end || '\";' from (select oid, proname, count(proname) over (partition by proname) cnt from pg_proc) tmpt order by '' || proname"

cat <<PROCSTR_FOOT
	}
	return "unknown proc";
}

PROCSTR_FOOT


echo
echo "#endif /* EXX_PGOID_GEN_HPP */"



