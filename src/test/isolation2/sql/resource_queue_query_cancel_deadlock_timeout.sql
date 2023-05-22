-- This test ensures that we gracefully handle the case where
-- deadlock_timeout elapses in the middle of query cancellation for a
-- query waiting on a resource queue. We must ensure that the
-- deadlock_timeout is disabled in such a situation to avoid a double
-- acquisition scenario of the partition LWLock which would result in
-- a PANIC.

-- Create a resource queue where only 1 query can run at a time and
-- new queries must wait. The role attached to the resource queue must
-- be a nonsuperuser.
0: CREATE RESOURCE QUEUE rq_query_cancel WITH (active_statements = 1);
0: CREATE ROLE role_rq_query_cancel RESOURCE QUEUE rq_query_cancel;

-- Inject a sleep fault of significant duration (6 seconds) to suspend
-- execution right after the partition lock has been acquired in
-- ResLockWaitCancel(). We cannot use a suspend fault because we'll be
-- holding the partition lock for resource queues.
0: SELECT gp_inject_fault('reslock_wait_cancel_after_acquire_partition_lock', 'sleep', '', '', '', 1, -1, 6, dbid) FROM gp_segment_configuration WHERE content = -1 AND role = 'p';
0&: SELECT gp_wait_until_triggered_fault('reslock_wait_cancel_after_acquire_partition_lock', 1, dbid) FROM gp_segment_configuration WHERE content = -1 AND role = 'p';

-- Session 1 will hold the active resource queue slot by sleeping for
-- a large duration - the duration of the test (until it is cancelled
-- at the very end during test clean up).
1: SET ROLE role_rq_query_cancel;
1&: SELECT pg_sleep(300);

-- Session 2 will try to acquire resource queue lock and wait in
-- queue. We will be ensuring that no "Waiting on lock already held"
-- PANIC occurs if deadlock_timeout elapses in the middle of query
-- cancellation (specifically, while the partition lock has been
-- acquired in ResLockWaitCancel()). We increase the deadlock_timeout
-- to give a reliable amount of time for Session 2 to trigger the
-- sleep fault. Note that the 3 second timer starts immediately before
-- the query starts its semaphore sleep in ResProcSleep() so an
-- external query cancel must happen with haste.
2: SET deadlock_timeout = '3s';
2: SET ROLE role_rq_query_cancel;
2&: SELECT 1;

-- Cancel Session 2's query which will trigger the sleep fault in
-- ResLockWaitCancel() right after the partition lock is acquired.
3: SELECT pg_cancel_backend(pid) FROM pg_stat_activity WHERE query = 'SELECT 1;';

-- Now that the sleep fault has been triggered, wait for 3 seconds to
-- see if the deadlock check gets triggered on Session 2. If we have
-- not done the right thing of disabling the check, it will lead to
-- the aforementioned double partition lock acquisition PANIC.
0<:
0: SELECT pg_sleep(3);
0: SELECT gp_inject_fault('reslock_wait_cancel_after_acquire_partition_lock', 'reset', dbid) FROM gp_segment_configuration WHERE content = -1 AND role = 'p';
2<:

-- Clean up the test
3: SELECT pg_cancel_backend(pid) FROM pg_stat_activity WHERE query = 'SELECT pg_sleep(300);';
1<:
0: DROP ROLE role_rq_query_cancel;
0: DROP RESOURCE QUEUE rq_query_cancel;
