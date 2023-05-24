aggop.h -- operation definition exchange between PG and XRG

aggref.i.c -- external interface between PG and KITE in master node
aggref_xexpr.c --  convert AGG plan to XEXPR format in master node

kite_client.c -- handle sockstream and get result from kite

kite_json.c -- create kite JSON from xexpr and PG plan

kite_target.c -- create each column/target from the PG targetlist.  Each target contains the mapping between index of targetlist and index of result from kite, column expression for SQL and the decode function.

kite_extscan.c -- Kite external scan structure run in segment node

exttab.i.c -- External Table GPDB interface

decode.c -- decode functions for all data types

calling sequence:

```
[Master Node]
if AGG plan found then
   generate XEXPR from AGG plan in master node (exx_aggref.c)
endif

[Child Node]
if AGG XEXPR found then
   convert AGG XEXPR to list of kite_target_t  (targetlist)
else
   Simple Table Scan to list of kite_target_t (targetlist)
endif

bool get_next() {
   if (res == NULL) {
      res = kite_get_result(sockstream);
      if (! res) {
          return false; // no more rows
      }
   }

   iter = kite_get_next(res);
   if (! iter) {
        kite_result_destroy(res);

	res = kite_get_result(sockstream);  // get result from socket
        if (res == NULL) {
           return false;  // no more rows
        }

        iter = kite_get_next(res);
        if (! iter) {
           return false;  // no more rows
        }
   }

   // have new rows
   int i = 0;
   for (target in targetlist) {
       target->decode(target, &datum[i], &isnull[i])  // decode and return the datum and isnull for a particular column i
       i++;
   }

   return true;
}


```
