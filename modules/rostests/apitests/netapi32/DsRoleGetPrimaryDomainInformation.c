/*
 * PROJECT:     ReactOS netapi32.dll API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for DsRoleGetPrimaryDomainInformation
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 *              Copyright 2018-2020 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <dsrole.h>

// #define NDEBUG
// #include <debug.h>

#define PRIMARYDOMAININFOBASIC_FLAGS_MASK ( \
    DSROLE_PRIMARY_DS_RUNNING             | \
    DSROLE_PRIMARY_DS_MIXED_MODE          | \
    DSROLE_UPGRADE_IN_PROGRESS            | \
    DSROLE_PRIMARY_DS_READONLY            | \
    DSROLE_PRIMARY_DOMAIN_GUID_PRESENT    )

static
void
test_InvalidArgs(void)
{
    DWORD dwErrorCode;
    PBYTE pInfo;

    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRoleUpgradeStatus, NULL);
    ok(dwErrorCode == ERROR_INVALID_PARAMETER,
       "dwErrorCode: expected ERROR_INVALID_PARAMETER, got %lu\n", dwErrorCode);

    pInfo = (PBYTE)0xdeadbeef;
    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic - 1, &pInfo);
    ok(dwErrorCode == ERROR_INVALID_PARAMETER,
       "dwErrorCode: expected ERROR_INVALID_PARAMETER, got %lu\n", dwErrorCode);
    ok(pInfo == NULL, "pInfo is not NULL\n");
    pInfo = (PBYTE)0xdeadbeef;
    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRoleOperationState + 1, &pInfo);
    ok(dwErrorCode == ERROR_INVALID_PARAMETER,
       "dwErrorCode: expected ERROR_INVALID_PARAMETER, got %lu\n", dwErrorCode);
    ok(pInfo == NULL, "pInfo is not NULL\n");
}

static
void
test_DsRoleOperationState(void)
{
    DWORD dwErrorCode;
    PDSROLE_OPERATION_STATE_INFO pInfo = NULL;

    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRoleOperationState, (PBYTE *)&pInfo);
    ok(dwErrorCode == ERROR_SUCCESS, "DsRoleGetPrimaryDomainInformation failed: %lu\n", dwErrorCode);
    if (pInfo == NULL)
    {
        skip("pInfo is NULL\n");
        return;
    }

    ok(pInfo->OperationState >= DsRoleOperationIdle &&
       pInfo->OperationState <= DsRoleOperationNeedReboot,
       "Invalid OperationState: %d\n", pInfo->OperationState);

    DsRoleFreeMemory(pInfo);
}

static
void
test_DsRolePrimaryDomainInfoBasic(void)
{
    DWORD dwErrorCode;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pInfo = NULL;

    // Get information about the domain membership of this computer.
    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (PBYTE *)&pInfo);
    ok(dwErrorCode == ERROR_SUCCESS, "DsRoleGetPrimaryDomainInformation failed: %lu\n", dwErrorCode);
    if (pInfo == NULL)
    {
        skip("pInfo is NULL\n");
        return;
    }

    ok(pInfo->MachineRole >= DsRole_RoleStandaloneWorkstation &&
       pInfo->MachineRole <= DsRole_RolePrimaryDomainController,
       "Invalid MachineRole: %d\n", pInfo->MachineRole);

    ok((pInfo->Flags & ~PRIMARYDOMAININFOBASIC_FLAGS_MASK) == 0,
       "Invalid Flags: 0x%08lx\n", pInfo->Flags);
    ok((pInfo->Flags & DSROLE_PRIMARY_DS_MIXED_MODE) == 0 ||
       (pInfo->Flags & DSROLE_PRIMARY_DS_RUNNING) != 0,
       "Mismatched Flags: 0x%08lx\n", pInfo->Flags);

    ok(pInfo->DomainNameFlat != NULL,
       "Invalid DomainNameFlat: NULL\n");

    // DomainNameDns and DomainForestName are optional.

    // TODO: Check DomainGuid is valid, if present.

    DsRoleFreeMemory(pInfo);
}

static
void
test_DsRoleUpgradeStatus(void)
{
    DWORD dwErrorCode;
    PDSROLE_UPGRADE_STATUS_INFO pInfo = NULL;

    dwErrorCode = DsRoleGetPrimaryDomainInformation(NULL, DsRoleUpgradeStatus, (PBYTE *)&pInfo);
    ok(dwErrorCode == ERROR_SUCCESS, "DsRoleGetPrimaryDomainInformation failed: %lu\n", dwErrorCode);
    if (pInfo == NULL)
    {
        skip("pInfo is NULL\n");
        return;
    }

    ok(pInfo->OperationState == 0 ||
       pInfo->OperationState == DSROLE_UPGRADE_IN_PROGRESS,
       "Invalid OperationState: %lu\n", pInfo->OperationState);

    ok(pInfo->PreviousServerState >= DsRoleServerUnknown &&
       pInfo->PreviousServerState <= DsRoleServerBackup,
       "Invalid PreviousServerState: %d\n", pInfo->PreviousServerState);

    DsRoleFreeMemory(pInfo);
}

START_TEST(DsRoleGetPrimaryDomainInformation)
{
    test_InvalidArgs();

    test_DsRoleOperationState();
    test_DsRolePrimaryDomainInfoBasic();
    test_DsRoleUpgradeStatus();
}
