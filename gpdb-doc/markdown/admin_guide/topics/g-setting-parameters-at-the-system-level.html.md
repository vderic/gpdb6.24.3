---
title: Setting Parameters at the System Level 
---

Master parameter settings in the master `postgresql.conf`file are the system-wide default. To set a master parameter:

1.  Edit the `$MASTER_DATA_DIRECTORY/postgresql.conf` file.
2.  Find the parameter to set, uncomment it \(remove the preceding `#` character\), and type the desired value.
3.  Save and close the file.
4.  For *session* parameters that do not require a server restart, upload the `postgresql.conf` changes as follows:

    ```
    $ gpstop -u
    ```

5.  For parameter changes that require a server restart, restart Greenplum Database as follows:

    ```
    $ gpstop -r
    ```


For details about the server configuration parameters, see the *Greenplum Database Reference Guide*.

**Parent topic:** [Setting a Master Configuration Parameter](../topics/g-setting-a-master-configuration-parameter.html)

