---
title: Upgrading to Greenplum 6 
---

This topic identifies the upgrade paths for upgrading a Greenplum Database 6.x release to a newer 6.x release. The topic also describes the migration paths for migrating VMware Greenplum Database 4.x or 5.x data to Greenplum Database 6.x.

Greenplum Database 6 supports upgrading from a Greenplum 6.x release to a newer 6.x release. Direct upgrade from Tanzu Greenplum 5.28.10 to Tanzu Greenplum 6.24.0 and later is provided via gpupgrade; for more information, see the [gpupgrade documentation](https://docs.vmware.com/en/VMware-Greenplum-Upgrade/index.html). Direct upgrade from Greenplum Database 4.3, or from Greenplum 5.27 and earlier, to Greenplum 6 is not supported; you must migrate the data to Greenplum 6.

-   **[Upgrading from an Earlier Greenplum 6 Release](upgrading.html)**  
The upgrade path supported for this release is Greenplum Database 6.x to a newer Greenplum Database 6.x release.
-   **[PXF Upgrade and Migration](../pxf/pxf_upgrade_migration.html)**  

-   **[Migrating Data from Greenplum 4.3 or 5 to Greenplum 6](migrate.html)**  
You can migrate data from Greenplum Database 4.3 or 5 to Greenplum 6 using the standard backup and restore procedures, `gpbackup` and `gprestore`, or by using `gpcopy` for VMware Greenplum.

**Parent topic:** [Installing and Upgrading Greenplum](install_guide.html)

