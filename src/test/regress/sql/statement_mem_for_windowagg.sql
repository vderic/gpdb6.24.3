CREATE TABLE dummy_table(x int, y int) DISTRIBUTED BY (y);
INSERT INTO dummy_table SELECT generate_series(0, 20000), 0;
INSERT INTO dummy_table SELECT generate_series(0, 20000), 3;
INSERT INTO dummy_table SELECT generate_series(0, 20000), 10;

-- 1. Test that if we set statement_mem to a larger value, the tuplestore
-- for caching the tuples in partition used in WindowAgg is able to be fitted
-- in memory.
SET statement_mem TO '4096kB';

SELECT gp_inject_fault('winagg_after_spool_tuples', 'skip', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

SELECT COUNT(*)
  FROM (SELECT AVG(x) OVER (PARTITION BY y) FROM dummy_table) tt;

SELECT gp_inject_fault('winagg_after_spool_tuples', 'reset', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

-- 2. Test that if we set statement_mem to a smaller value, the tuplestore
-- for caching the tuples in partition used in WindowAgg will be spilled to disk.
SET statement_mem TO '1024kB';

SELECT gp_inject_fault('winagg_after_spool_tuples', 'skip', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

SELECT COUNT(*)
  FROM (SELECT AVG(x) OVER (PARTITION BY y) FROM dummy_table) tt;

SELECT gp_inject_fault('winagg_after_spool_tuples', 'reset', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

-- 3. Test that if we set statement_mem to a smaller value, the tuplesort
-- operation in DISTINCT-qualified WindowAgg will be spilled to disk.
SET statement_mem TO '1024kB';
-- MK-Sort
SET gp_enable_mk_sort TO 'on';
SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'skip', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

SELECT COUNT(*)
  FROM (SELECT AVG(DISTINCT x) OVER (PARTITION BY y) FROM dummy_table) tt;

SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'reset', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

-- PG-Sort
SET gp_enable_mk_sort TO 'off';
SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'skip', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

SELECT COUNT(*)
  FROM (SELECT AVG(DISTINCT x) OVER (PARTITION BY y) FROM dummy_table) tt;

SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'reset', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

-- 4. Test that if we set statement_mem to a larger value, the tuplesort
-- operation in DISTINCT-qualified WindowAgg is able to be fitted in memory.
SET statement_mem TO '4096kB';
-- MK-Sort
SET gp_enable_mk_sort TO 'on';
SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'skip', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

SELECT COUNT(*)
  FROM (SELECT AVG(DISTINCT x) OVER (PARTITION BY y) FROM dummy_table) tt;

SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'reset', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

-- PG-Sort
SET gp_enable_mk_sort TO 'off';
SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'skip', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;

SELECT COUNT(*)
  FROM (SELECT AVG(DISTINCT x) OVER (PARTITION BY y) FROM dummy_table) tt;

SELECT gp_inject_fault_infinite('distinct_winagg_perform_sort', 'reset', dbid)
  FROM gp_segment_configuration WHERE role='p' AND content>=0;
-- Do some clean-ups.
DROP TABLE dummy_table;
RESET statement_mem;
RESET gp_enable_mk_sort;
