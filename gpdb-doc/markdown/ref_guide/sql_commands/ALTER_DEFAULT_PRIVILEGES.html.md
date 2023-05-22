---
title: ALTER DEFAULT PRIVILEGES 
---

Changes default access privileges.

## <a id="section2"></a>Synopsis 

``` {#sql_command_synopsis}

ALTER DEFAULT PRIVILEGES
    [ FOR { ROLE | USER } <target_role> [, ...] ]
    [ IN SCHEMA <schema_name> [, ...] ]
    <abbreviated_grant_or_revoke>

where <abbreviated_grant_or_revoke> is one of:

GRANT { { SELECT | INSERT | UPDATE | DELETE | TRUNCATE | REFERENCES | TRIGGER }
    [, ...] | ALL [ PRIVILEGES ] }
    ON TABLES
    TO { [ GROUP ] <role_name> | PUBLIC } [, ...] [ WITH GRANT OPTION ]

GRANT { { USAGE | SELECT | UPDATE }
    [, ...] | ALL [ PRIVILEGES ] }
    ON SEQUENCES
    TO { [ GROUP ] <role_name> | PUBLIC } [, ...] [ WITH GRANT OPTION ]

GRANT { EXECUTE | ALL [ PRIVILEGES ] }
    ON FUNCTIONS
    TO { [ GROUP ] <role_name> | PUBLIC } [, ...] [ WITH GRANT OPTION ]

GRANT { USAGE | ALL [ PRIVILEGES ] }
    ON TYPES
    TO { [ GROUP ] <role_name> | PUBLIC } [, ...] [ WITH GRANT OPTION ]

REVOKE [ GRANT OPTION FOR ]
    { { SELECT | INSERT | UPDATE | DELETE | TRUNCATE | REFERENCES | TRIGGER }
    [, ...] | ALL [ PRIVILEGES ] }
    ON TABLES
    FROM { [ GROUP ] <role_name> | PUBLIC } [, ...]
    [ CASCADE | RESTRICT ]

REVOKE [ GRANT OPTION FOR ]
    { { USAGE | SELECT | UPDATE }
    [, ...] | ALL [ PRIVILEGES ] }
    ON SEQUENCES
    FROM { [ GROUP ] <role_name> | PUBLIC } [, ...]
    [ CASCADE | RESTRICT ]

REVOKE [ GRANT OPTION FOR ]
    { EXECUTE | ALL [ PRIVILEGES ] }
    ON FUNCTIONS
    FROM { [ GROUP ] <role_name> | PUBLIC } [, ...]
    [ CASCADE | RESTRICT ]

REVOKE [ GRANT OPTION FOR ]
    { USAGE | ALL [ PRIVILEGES ] }
    ON TYPES
    FROM { [ GROUP ] <role_name> | PUBLIC } [, ...]
    [ CASCADE | RESTRICT ]

```

## <a id="section3"></a>Description 

`ALTER DEFAULT PRIVILEGES` allows you to set the privileges that will be applied to objects created in the future. \(It does not affect privileges assigned to already-existing objects.\) Currently, only the privileges for tables \(including views and foreign tables\), sequences, functions, and types \(including domains\) can be altered.

You can change default privileges only for objects that will be created by yourself or by roles that you are a member of. The privileges can be set globally \(i.e., for all objects created in the current database\), or just for objects created in specified schemas. Default privileges that are specified per-schema are added to whatever the global default privileges are for the particular object type.

As explained under [GRANT](GRANT.html), the default privileges for any object type normally grant all grantable permissions to the object owner, and may grant some privileges to `PUBLIC` as well. However, this behavior can be changed by altering the global default privileges with `ALTER DEFAULT PRIVILEGES`.

## <a id="parm"></a>Parameters 

target\_role
:   The name of an existing role of which the current role is a member. If `FOR ROLE` is omitted, the current role is assumed.

schema\_name
:   The name of an existing schema. If specified, the default privileges are altered for objects later created in that schema. If `IN SCHEMA` is omitted, the global default privileges are altered.

role\_name
:   The name of an existing role to grant or revoke privileges for. This parameter, and all the other parameters in abbreviated\_grant\_or\_revoke, act as described under [GRANT](GRANT.html) or [REVOKE](REVOKE.html), except that one is setting permissions for a whole class of objects rather than specific named objects.

## <a id="sql-alterdefaultprivileges-notes"></a>Notes 

Use [psql](../../utility_guide/ref/psql.html)'s `\ddp` command to obtain information about existing assignments of default privileges. The meaning of the privilege values is the same as explained for `\dp` under [GRANT](GRANT.html).

If you wish to drop a role for which the default privileges have been altered, it is necessary to reverse the changes in its default privileges or use `DROP OWNED BY` to get rid of the default privileges entry for the role.

## <a id="sql-alterdefaultprivileges-examples"></a>Examples 

Grant SELECT privilege to everyone for all tables \(and views\) you subsequently create in schema `myschema`, and allow role `webuser` to INSERT into them too:

```

ALTER DEFAULT PRIVILEGES IN SCHEMA myschema GRANT SELECT ON TABLES TO PUBLIC;
ALTER DEFAULT PRIVILEGES IN SCHEMA myschema GRANT INSERT ON TABLES TO webuser;

```

Undo the above, so that subsequently-created tables won't have any more permissions than normal:

```

ALTER DEFAULT PRIVILEGES IN SCHEMA myschema REVOKE SELECT ON TABLES FROM PUBLIC;
ALTER DEFAULT PRIVILEGES IN SCHEMA myschema REVOKE INSERT ON TABLES FROM webuser;

```

Remove the public EXECUTE permission that is normally granted on functions, for all functions subsequently created by role `admin`:

```

ALTER DEFAULT PRIVILEGES FOR ROLE admin REVOKE EXECUTE ON FUNCTIONS FROM PUBLIC;

```

## <a id="compat"></a>Compatibility 

There is no `ALTER DEFAULT PRIVILEGES` statement in the SQL standard.

## <a id="see-also"></a>See Also 

[GRANT](GRANT.html), [REVOKE](REVOKE.html)

**Parent topic:** [SQL Commands](../sql_commands/sql_ref.html)

