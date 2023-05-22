# gprecoverseg 

Recovers a primary or mirror segment instance that has been marked as down \(if mirroring is enabled\).

## <a id="section2"></a>Synopsis 

```
gprecoverseg [[-p <new_recover_host>[,...]] | -i <recover_config_file>] [-d <master_data_directory>] 
             [-b <segment_batch_size>] [-B <batch_size>] [-F [-s]] [-a] [-q] 
	      [--hba-hostnames <boolean>] 
             [--no-progress] [-l <logfile_directory>]

gprecoverseg -r 

gprecoverseg -o <output_recover_config_file> 
             [-p <new_recover_host>[,...]]

gprecoverseg -? | -h | --help
        
gprecoverseg -v | --verbose

gprecoverseg --version
```

## <a id="section3"></a>Description 

In a system with mirrors enabled, the `gprecoverseg` utility reactivates a failed segment instance and identifies the changed database files that require resynchronization. Once `gprecoverseg` completes this process, the system goes into `Not in Sync` mode until the recovered segment is brought up to date. The system is online and fully operational during resynchronization.

During an incremental recovery \(the `-F` option is not specified\), if `gprecoverseg` detects a segment instance with mirroring deactivated in a system with mirrors activated, the utility reports that mirroring is deactivated for the segment, does not attempt to recover that segment instance, and continues the recovery process.

A segment instance can fail for several reasons, such as a host failure, network failure, or disk failure. When a segment instance fails, its status is marked as `d` \(down\) in the Greenplum Database system catalog, and its mirror is activated in `Not in Sync` mode. In order to bring the failed segment instance back into operation again, you must first correct the problem that made it fail in the first place, and then recover the segment instance in Greenplum Database using `gprecoverseg`.

> **Note** If incremental recovery was not successful and the down segment instance data is not corrupted, contact VMware Support.

Segment recovery using `gprecoverseg` requires that you have an active mirror to recover from. For systems that do not have mirroring enabled, or in the event of a double fault \(a primary and mirror pair both down at the same time\) — you must take manual steps to recover the failed segment instances and then perform a system restart to bring the segments back online. For example, this command restarts a system.

```
gpstop -r
```

By default, a failed segment is recovered in place, meaning that the system brings the segment back online on the same host and data directory location on which it was originally configured. In this case, use the following format for the recovery configuration file \(using `-i`\).

```
<failed_host_address>|<port>|<data_directory> 
```

In some cases, this may not be possible \(for example, if a host was physically damaged and cannot be recovered\). In this situation, `gprecoverseg` allows you to recover failed segments to a completely new host \(using `-p`\), on an alternative data directory location on your remaining live segment hosts \(using `-s`\), or by supplying a recovery configuration file \(using `-i`\) in the following format. The word <SPACE\> indicates the location of a required space. Do not add additional spaces.

```
<failed_host_address>|<port>|<data_directory><SPACE>
<recovery_host_address>|<port>|<data_directory>

```

See the `-i` option below for details and examples of a recovery configuration file.

The `gp_segment_configuration` system catalog table can help you determine your current segment configuration so that you can plan your mirror recovery configuration. For example, run the following query:

```
=# SELECT dbid, content, address, port, datadir 
   FROM gp_segment_configuration
   ORDER BY dbid;
```

The new recovery segment host must be pre-installed with the Greenplum Database software and configured exactly the same as the existing segment hosts. A spare data directory location must exist on all currently configured segment hosts and have enough disk space to accommodate the failed segments.

The recovery process marks the segment as up again in the Greenplum Database system catalog, and then initiates the resynchronization process to bring the transactional state of the segment up-to-date with the latest changes. The system is online and available during `Not in Sync` mode.

## <a id="section4"></a>Options 

-a \(do not prompt\)
:   Do not prompt the user for confirmation.

-b segment\_batch\_size
:   The maximum number of segments per host to operate on in parallel. Valid values are `1` to `128`. If not specified, the utility will start recovering up to 64 segments in parallel on each host.

-B batch\_size
:   The number of hosts to work on in parallel. If not specified, the utility will start working on up to 16 hosts in parallel. Valid values are `1` to `64`.

-d master\_data\_directory
:   Optional. The master host data directory. If not specified, the value set for `$MASTER_DATA_DIRECTORY` will be used.

-F \(full recovery\)
:   Optional. Perform a full copy of the active segment instance in order to recover the failed segment. The default is to only copy over the incremental changes that occurred while the segment was down.

    **Warning:** A full recovery deletes the data directory of the down segment instance before copying the data from the active \(current primary\) segment instance. Before performing a full recovery, ensure that the segment failure did not cause data corruption and that any host segment disk issues have been fixed.

    Also, for a full recovery, the utility does not restore custom files that are stored in the segment instance data directory even if the custom files are also in the active segment instance. You must restore the custom files manually. For example, when using the `gpfdists` protocol \(`gpfdist` with SSL encryption\) to manage external data, client certificate files are required in the segment instance `$PGDATA/gpfdists` directory. These files are not restored. For information about configuring `gpfdists`, see [Encrypting gpfdist Connections](../../security-guide/topics/Encryption.html).

    Use the `-s` option to output a new line once per second for each segment. Alternatively, use the `--no-progress` option to completely deactivate progress reports.

--hba-hostnames boolean
:   Optional. Controls whether this utility uses IP addresses or host names in the `pg_hba.conf` file when updating this file with addresses that can connect to Greenplum Database. When set to 0 -- the default value -- this utility uses IP addresses when updating this file. When set to 1, this utility uses host names when updating this file. For consistency, use the same value that was specified for `HBA_HOSTNAMES` when the Greenplum Database system was initialized. For information about how Greenplum Database resolves host names in the `pg_hba.conf` file, see [Configuring Client Authentication](../../admin_guide/client_auth.html).

-i recover\_config\_file
:   Specifies the name of a file with the details about failed segments to recover.

    Each line in the config file specifies a segment to recover. This line can have one of two formats. In the event of in-place \(incremental\) recovery, enter one group of pipe-delimited fields in the line. For example:

    ```
    failedAddress|failedPort|failedDataDirectory
    ```

    For recovery to a new location, enter two groups of fields separated by a space in the line. The required space is indicated by <SPACE\>. Do not add additional spaces.

    ```
    failedAddress|failedPort|failedDataDirectory<SPACE>newAddress|
    newPort|newDataDirectory
    ```

    > **Note** Lines beginning with `#` are treated as comments and ignored.

    **Examples**

    **In-place \(incremental\) recovery of a single mirror**

    ```
    sdw1-1|50001|/data1/mirror/gpseg16
    ```

    **Recovery of a single mirror to a new host**

    ```
    sdw1-1|50001|/data1/mirror/gpseg16<SPACE>sdw4-1|50001|/data1/recover1/gpseg16
    ```

    **Obtaining a Sample File**

    You can use the `-o` option to output a sample recovery configuration file to use as a starting point. The output file lists the currently invalid segments and their default recovery location. This file format can be used with the `-i` option for in-place \(incremental\) recovery.

-l logfile\_directory
:   The directory to write the log file. Defaults to `~/gpAdminLogs`.

-o output\_recover\_config\_file
:   Specifies a file name and location to output a sample recovery configuration file. This file can be edited to supply alternate recovery locations if needed. The following example outputs the default recovery configuration file:

    ```
    $ gprecoverseg -o /home/gpadmin/recover_config_file
    ```

-p new\_recover\_host\[,...\]
:   Specifies a new host outside of the currently configured Greenplum Database array on which to recover invalid segments.

:   The new host must have the Greenplum Database software installed and configured, and have the same hardware and OS configuration as the current segment hosts \(same OS version, locales, `gpadmin` user account, data directory locations created, ssh keys exchanged, number of network interfaces, network interface naming convention, and so on\). Specifically, the Greenplum Database binaries must be installed, the new host must be able to connect password-less with all segments including the Greenplum master, and any other Greenplum Database specific OS configuration parameters must be applied.

:   > **Note** In the case of multiple failed segment hosts, you can specify the hosts to recover with a comma-separated list. However, it is strongly recommended to recover one host at a time. If you must recover more than one host at a time, then it is critical to ensure that a double fault scenario does not occur, in which both the segment primary and corresponding mirror are offline.

-q \(no screen output\)
:   Run in quiet mode. Command output is not displayed on the screen, but is still written to the log file.

-r \(rebalance segments\)
:   After a segment recovery, segment instances may not be returned to the preferred role that they were given at system initialization time. This can leave the system in a potentially unbalanced state, as some segment hosts may have more active segments than is optimal for top system performance. This option rebalances primary and mirror segments by returning them to their preferred roles. All segments must be valid and resynchronized before running `gprecoverseg -r`. If there are any in progress queries, they will be cancelled and rolled back.

-s \(sequential progress\)
:   Show `pg_basebackup` or `pg_rewind` progress sequentially instead of in-place. Useful when writing to a file, or if a tty does not support escape sequences. The default is to show progress in-place.

--no-progress
:   Suppresses progress reports from the `pg_basebackup` or `pg_rewind` utility. The default is to display progress.

-v \| --verbose
:   Sets logging output to verbose.

--version
:   Displays the version of this utility.

-? \| -h \| --help
:   Displays the online help.

## <a id="section5"></a>Examples 

**Example 1: Recover Failed Segments in Place**

Recover any failed segment instances in place:

```
$ gprecoverseg
```

**Example 2: Rebalance Failed Segments If Not in Preferred Roles**

First, verify that all segments are up and running, reysynchronization has completed, and there are segments **not** in preferred roles:

```
$ gpstate -e
```

Then, if necessary, rebalance the segments:

```
$ gprecoverseg -r
```

**Example 3: Recover Failed Segments to a Separate Host**

Recover any failed segment instances to a newly configured new segment host:

```
$ gprecoverseg -i <recover_config_file>
```

## <a id="section6"></a>See Also 

[gpstart](gpstart.html), [gpstop](gpstop.html)

