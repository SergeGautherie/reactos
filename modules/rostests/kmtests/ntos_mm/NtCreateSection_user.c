/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite NtCreateSection test user-mode part
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 * + SG
 */

#include <kmt_test.h>

static UNICODE_STRING InitOnCreate = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-NtCreateSection\\InitOnCreate");
static UNICODE_STRING InitOnRW = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-NtCreateSection\\InitOnRW");
static UNICODE_STRING InvalidInit = RTL_CONSTANT_STRING(L"\\Device\\Kmtest-NtCreateSection\\InvalidInit");

static
VOID
TestN(
    _In_ UINT TestNumber,
    _In_ ULONG CreateDisposition,
    _In_ PUNICODE_STRING ObjectName,
    _In_ size_t TestSize
)
{
    PVOID Buffer;
    ULONG FileSize;
    NTSTATUS Status;
    LARGE_INTEGER MaxFileSize;
    HANDLE Handle, SectionHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;

    trace("Test %u\n", TestNumber);

//    trace("Calling InitializeObjectAttributes\n");
    InitializeObjectAttributes(&ObjectAttributes, ObjectName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    trace("Calling NtCreateFile\n");
    // Triggers IRP_MJ_CREATE.
    Status = NtCreateFile(&Handle, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &ObjectAttributes, &IoStatusBlock,
                          NULL, FILE_ATTRIBUTE_NORMAL, 0, CreateDisposition, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);

    trace("Calling NtCreateSection\n");
    MaxFileSize.QuadPart = TestSize;
    // Triggers, if not InvalidInit, 1-2 IRP_MJ_QUERY_INFORMATION + IRP_MJ_SET_INFORMATION.
    Status = NtCreateSection(&SectionHandle, SECTION_ALL_ACCESS, 0, &MaxFileSize,
                             PAGE_READWRITE, SEC_COMMIT, Handle);

    if (RtlCompareUnicodeString(ObjectName, &InvalidInit, FALSE) == 0)
    {

    trace("STATUS_INVALID_FILE_FOR_SECTION case\n");
    ok_eq_hex(Status, STATUS_INVALID_FILE_FOR_SECTION);
    if (NT_SUCCESS(Status))
        NtClose(SectionHandle);

    }
    else
    {

    trace("STATUS_SUCCESS case\n");
    ok_eq_hex(Status, STATUS_SUCCESS);

//    trace("Calling NtMapViewOfSection\n");
    Buffer = NULL;
    FileSize = 0;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(), &Buffer, 0, 0, 0,
                                &FileSize, ViewUnmap, 0, PAGE_READWRITE);
    ok_eq_hex(Status, STATUS_SUCCESS);

//    trace("Calling KmtStartSeh\n");
    KmtStartSeh();
    trace("Checking ((PCHAR)Buffer)[0] == 0\n");
    // Triggers IRP_MJ_READ.
    ok(((PCHAR)Buffer)[0] == 0, "First byte is not null! %x\n", ((PCHAR)Buffer)[0]);
    trace("Calling memset\n");
    memset(Buffer, 0xBA, TestSize);
    trace("Calling KmtEndSeh\n");
    KmtEndSeh(STATUS_SUCCESS);

//    trace("Calling NtUnmapViewOfSection\n");
    Status = NtUnmapViewOfSection(NtCurrentProcess(), Buffer);
    ok_eq_hex(Status, STATUS_SUCCESS);
//    trace("Calling NtClose(SectionHandle)\n");
    Status = NtClose(SectionHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    } // else, if (RtlCompareUnicodeString(ObjectName, &InvalidInit, FALSE) == 0)

    trace("Calling NtClose(Handle)\n");
    // Triggers IRP_MJ_CLEANUP.
    Status = NtClose(Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);
}

START_TEST(NtCreateSection)
{
//    trace("Calling KmtLoadDriver\n");
    KmtLoadDriver(L"NtCreateSection", FALSE);
    trace("Calling KmtOpenDriver\n");
    // IRP_MJ_CREATE: init.
    KmtOpenDriver();

//    // 37094
//    // IRP_MJ_CREATE: no init. NtCreateSection: STATUS_INVALID_FILE_FOR_SECTION. IRP_MJ_CLEANUP only: skip uninit.
    TestN( 0, FILE_CREATE, &InvalidInit ,  512);
////    // 193 37087
////    // IRP_MJ_CREATE: init. IRP_MJ_READ: STATUS_END_OF_FILE (0 + 4096 > 0). IRP_MJ_CLEANUP + IRP_MJ_WRITE: uninit.
    TestN( 1, FILE_CREATE, &InitOnCreate,  512);
    TestN( 2, FILE_CREATE, &InitOnCreate, 4096);
//    // 190 37084
//    // IRP_MJ_CREATE: no init. IRP_MJ_READ: STATUS_END_OF_FILE (0 + 4096 > 0). IRP_MJ_CLEANUP only: force uninit.
    TestN( 3, FILE_CREATE, &InitOnRW    ,  512);
    TestN( 4, FILE_CREATE, &InitOnRW    , 4096);
    TestN(10, FILE_OPEN  , &InvalidInit ,  512);
    TestN(11, FILE_OPEN  , &InitOnCreate,  512);
    TestN(12, FILE_OPEN  , &InitOnCreate, 4096);
    TestN(13, FILE_OPEN  , &InitOnRW    ,  512);
    TestN(14, FILE_OPEN  , &InitOnRW    , 4096);

    trace("Calling KmtCloseDriver\n");
    // IRP_MJ_CLEANUP: unint.
    KmtCloseDriver();
//    trace("Calling KmtUnloadDriver\n");
    KmtUnloadDriver();
}
