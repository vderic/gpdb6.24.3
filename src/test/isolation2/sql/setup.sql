CREATE OR REPLACE LANGUAGE plpythonu;

-- Helper function, to call either __gp_aoseg, or gp_aocsseg, depending
-- on whether the table is row- or column-oriented. This allows us to
-- run the same test queries on both.
--
-- The Python utility that runs this doesn't know about dollar-quoting,
-- and thinks that a ';' at end of line ends the command. The /* in func */
-- comments at the end of each line thwarts that.
CREATE OR REPLACE FUNCTION gp_ao_or_aocs_seg(rel regclass,
  segno OUT integer,
  tupcount OUT bigint,
  modcount OUT bigint,
  formatversion OUT smallint,
  state OUT smallint)
RETURNS SETOF record as $$
declare
  relstorage_var char;	/* in func */
begin	/* in func */
  select relstorage into relstorage_var from pg_class where oid = rel; /* in func */
  if relstorage_var = 'c' then	/* in func */
    for segno, tupcount, modcount, formatversion, state in SELECT DISTINCT x.segno, x.tupcount, x.modcount, x.formatversion, x.state FROM gp_toolkit.__gp_aocsseg(rel) x loop	/* in func */
      return next;	/* in func */
    end loop;	/* in func */
  else	/* in func */
    for segno, tupcount, modcount, formatversion, state in SELECT x.segno, x.tupcount, x.modcount, x.formatversion, x.state FROM gp_toolkit.__gp_aoseg(rel) x loop	/* in func */
      return next;	/* in func */
    end loop;	/* in func */
  end if;	/* in func */
end;	/* in func */
$$ LANGUAGE plpgsql;

-- Show locks in master and in segments. Because the number of segments
-- in the cluster depends on configuration, we print only summary information
-- of the locks in segments. If a relation is locked only on one segment,
-- we print that as a special case, but otherwise we just print "n segments",
-- meaning the relation is locked on more than one segment.
create or replace view locktest_master as
select coalesce(
  case when relname like 'pg_toast%index' then 'toast index'
       when relname like 'pg_toast%' then 'toast table'
       when relname like 'pg_aoseg%' then 'aoseg table'
       when relname like 'pg_aovisimap%index' then 'aovisimap index'
       when relname like 'pg_aovisimap%' then 'aovisimap table'
       else relname end, 'dropped table'),
  mode,
  locktype,
  'master'::text as node
from pg_locks l
left outer join pg_class c on ((l.locktype = 'append-only segment file' and l.relation = c.relfilenode) or (l.locktype != 'append-only segment file' and l.relation = c.oid)),
pg_database d
where relation is not null
and l.database = d.oid
and (relname <> 'gp_fault_strategy' and relname != 'locktest_master' or relname is NULL)
and d.datname = current_database()
and l.gp_segment_id = -1
group by l.gp_segment_id, relation, relname, locktype, mode
order by 1, 3, 2;

create or replace view locktest_segments_dist as
select relname,
  mode,
  locktype,
  l.gp_segment_id as node,
  relation
from pg_locks l
left outer join pg_class c on ((l.locktype = 'append-only segment file' and l.relation = c.relfilenode) or (l.locktype != 'append-only segment file' and l.relation = c.oid)),
pg_database d
where relation is not null
and l.database = d.oid
and (relname <> 'gp_fault_strategy' and relname != 'locktest_segments_dist' or relname is NULL)
and d.datname = current_database()
and l.gp_segment_id > -1
group by l.gp_segment_id, relation, relname, locktype, mode;

create or replace view locktest_segments as
SELECT coalesce(
  case when relname like 'pg_toast%index' then 'toast index'
       when relname like 'pg_toast%' then 'toast table'
       when relname like 'pg_aoseg%' then 'aoseg table'
       when relname like 'pg_aovisimap%index' then 'aovisimap index'
       when relname like 'pg_aovisimap%' then 'aovisimap table'
       else relname end, 'dropped table'),
  mode,
  locktype,
  case when count(*) = 1 then '1 segment'
       else 'n segments' end as node
  FROM gp_dist_random('locktest_segments_dist')
  group by relname, relation, mode, locktype;

-- Helper function
CREATE or REPLACE FUNCTION wait_until_waiting_for_required_lock (rel_name text, lmode text, segment_id integer) /*in func*/
RETURNS bool AS
$$
declare
retries int; /* in func */
begin /* in func */
  retries := 1200; /* in func */
  loop /* in func */
    if (select not granted from pg_locks l where granted='f' and l.relation::regclass = rel_name::regclass and l.mode=lmode and l.gp_segment_id=segment_id) then /* in func */
      return true; /* in func */
    end if; /* in func */
    if retries <= 0 then /* in func */
      return false; /* in func */
    end if; /* in func */
    perform pg_sleep(0.1); /* in func */
    retries := retries - 1; /* in func */
  end loop; /* in func */
end; /* in func */
$$ language plpgsql;

CREATE OR REPLACE FUNCTION is_query_waiting_for_syncrep(iterations int, check_query text) RETURNS bool AS $$
    for i in range(iterations):
        results = plpy.execute("SELECT * FROM\
                                (SELECT gp_execution_segment() AS gp_segment_id, query, waiting_reason\
                                FROM gp_dist_random('pg_stat_activity')) s\
                                WHERE gp_segment_id = 1 AND\
                                query = '%s' AND\
                                waiting_reason = 'replication'" % check_query )
        if results:
            return True
    return False
$$ LANGUAGE plpythonu VOLATILE;

-- Helper function that ensures mirror of the specified contentid is down.
create or replace function wait_for_mirror_down(contentid smallint, timeout_sec integer) returns bool as
$$
declare i int; /* in func */
begin /* in func */
    i := 0; /* in func */
    loop /* in func */
        perform gp_request_fts_probe_scan(); /* in func */
        if (select count(1) from gp_segment_configuration where role='m' and content=$1 and status='d') = 1 then /* in func */
            return true; /* in func */
        end if; /* in func */
        if i >= 2 * $2 then /* in func */
            return false; /* in func */
        end if; /* in func */
        perform pg_sleep(0.5); /* in func */
        i = i + 1; /* in func */
    end loop; /* in func */
end; /* in func */
$$ language plpgsql;
