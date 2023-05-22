---
title: CREATE GROUP 
---

Defines a new database role.

## <a id="section2"></a>Synopsis 

``` {#sql_command_synopsis}
CREATE GROUP <name> [[WITH] <option> [ ... ]]
```

where option can be:

```
      SUPERUSER | NOSUPERUSER
    | CREATEDB | NOCREATEDB
    | CREATEROLE | NOCREATEROLE
    | CREATEUSER | NOCREATEUSER
    | CREATEEXTTABLE | NOCREATEEXTTABLE 
      [ ( <attribute>='<value>'[, ...] ) ]
           where <attributes> and <value> are:
           type='readable'|'writable'
           protocol='gpfdist'|'http'
    | INHERIT | NOINHERIT
    | LOGIN | NOLOGIN
    | CONNECTION LIMIT <connlimit>
    | [ ENCRYPTED | UNENCRYPTED ] PASSWORD '<password>'
    | VALID UNTIL '<timestamp>' 
    | IN ROLE <rolename> [, ...]
    | ROLE <rolename> [, ...]
    | ADMIN <rolename> [, ...]
    | RESOURCE QUEUE <queue_name>
    | RESOURCE GROUP <group_name>
    | [ DENY <deny_point> ]
    | [ DENY BETWEEN <deny_point> AND <deny_point>]
```

## <a id="section3"></a>Description 

`CREATE GROUP` is an alias for [CREATE ROLE](CREATE_ROLE.html).

## <a id="section4"></a>Compatibility 

There is no `CREATE GROUP` statement in the SQL standard.

## <a id="section5"></a>See Also 

[CREATE ROLE](CREATE_ROLE.html)

**Parent topic:** [SQL Commands](../sql_commands/sql_ref.html)

