/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Io Regressions KM-Test (Mdl)
 * PROGRAMMER:      Aleksey Bragin <aleksey@reactos.org>
 *                  Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <kmt_test.h>

// #include <stdio.h>
#include <stdlib.h>
/*
#define NDEBUG
#include <debug.h>
*/

// Default value of MDL.MdlFlags.
#define MDL_NONE 0x0000

// Define to enable Aleksey's tests.
// Mostly redundant (as legacy) with enabled tests.
// #define ALEKSEY_TESTS

// Define to enable intermediate tests.
// Redundant (as confirmation) with enabled tests.
// #define INTERMEDIATE_TESTS

static USHORT g_NtVersion;

/*
// CSHORT should be USHORT, not short !!?
// MSDN types page only lists CCHAR.
https://git.reactos.org/?p=reactos.git;a=blob;f=sdk/include/xdk/ntbasedef.h;hb=HEAD
 507 /* Cardinal Data Types * /
 508 typedef char CCHAR;
 509 $if(_NTDEF_)
 510 typedef CCHAR *PCCHAR;
 511 typedef short CSHORT, *PCSHORT;
 512 typedef ULONG CLONG, *PCLONG;
 513 $endif(_NTDEF_)
*/
/*
https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/ns-wdm-_mdl
+
https://git.reactos.org/?p=reactos.git;a=blob;f=sdk/include/xdk/mmtypes.h;hb=HEAD#l105
 106 _Struct_size_bytes_(_Inexpressible_(sizeof(struct _MDL) +
 107     (ByteOffset + ByteCount + PAGE_SIZE-1) / PAGE_SIZE * sizeof(PFN_NUMBER)))
 108 typedef struct _MDL {
 109   struct _MDL *Next;
 110   CSHORT Size;
 111   CSHORT MdlFlags;
 112   struct _EPROCESS *Process;
 113   PVOID MappedSystemVa;
 114   PVOID StartVa;
 115   ULONG ByteCount;
 116   ULONG ByteOffset;
 117 } MDL, *PMDL;

https://git.reactos.org/?p=reactos.git;a=blob;f=sdk/include/xdk/mmfuncs.h;hb=HEAD
 153 #define MmInitializeMdl(_MemoryDescriptorList, \
 154                         _BaseVa, \
 155                         _Length) \
 156 { \
 157   (_MemoryDescriptorList)->Next = (PMDL) NULL; \
 158   (_MemoryDescriptorList)->Size = (CSHORT) (sizeof(MDL) + \
 159     (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES(_BaseVa, _Length))); \
 160   (_MemoryDescriptorList)->MdlFlags = 0; \
 161   (_MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN(_BaseVa); \
 162   (_MemoryDescriptorList)->ByteOffset = BYTE_OFFSET(_BaseVa); \
 163   (_MemoryDescriptorList)->ByteCount = (ULONG) _Length; \
 164 }
*/
// Check size here, until it is done in a "dedicated" test.
#ifndef _WIN64
// 12 + (4 * sizeof(PVOID)).
C_ASSERT(sizeof(MDL) == 28);
#else
// Includes '+ 4', as Process is aligned.
C_ASSERT(sizeof(MDL) == 48);
#endif

/*
https://git.reactos.org/?p=reactos.git;a=blob;f=sdk/include/xdk/mmtypes.h;hb=HEAD
  18 #define MDL_MAPPED_TO_SYSTEM_VA     0x0001
  19 #define MDL_PAGES_LOCKED            0x0002
  20 #define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
  21 #define MDL_ALLOCATED_FIXED_SIZE    0x0008
  22 #define MDL_PARTIAL                 0x0010
  23 #define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
  24 #define MDL_IO_PAGE_READ            0x0040
  25 #define MDL_WRITE_OPERATION         0x0080
  26 #define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
  27 #define MDL_FREE_EXTRA_PTES         0x0200
  28 #define MDL_DESCRIBES_AWE           0x0400
  29 #define MDL_IO_SPACE                0x0800
  30 #define MDL_NETWORK_HEADER          0x1000
  31 #define MDL_MAPPING_CAN_FAIL        0x2000
  32 #define MDL_ALLOCATED_MUST_SUCCEED  0x4000
  33 #define MDL_INTERNAL                0x8000
  34 
  35 #define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
  36                            MDL_PAGES_LOCKED            | \
  37                            MDL_SOURCE_IS_NONPAGED_POOL | \
  38                            MDL_PARTIAL_HAS_BEEN_MAPPED | \
  39                            MDL_PARENT_MAPPED_SYSTEM_VA | \
  40                            MDL_SYSTEM_VA               | \
  41                            MDL_IO_SPACE)
*/

#define CHECK_MDL(Mdl, VirtualAddress, Length, Size, MdlFlags) \
    CheckMdl(__LINE__, Mdl, VirtualAddress, Length, \
             Size, MdlFlags)

// Check fields of the allocated struct.
static
VOID
CheckMdl(INT line, PMDL Mdl, PVOID VirtualAddress, ULONG Length,
         USHORT Size, USHORT MdlFlags)
{
    MDL MdlCopy;

    memcpy(&MdlCopy, Mdl, sizeof(MdlCopy));

    // 
/*
    trace("(TLine=%d) Mdl=0x%p L=%lu S=%hu F=0x%04hx P=0x%p M=0x%p O=%lu\n",
          line, Mdl, Length,
          (USHORT)Mdl->Size, Mdl->MdlFlags, Mdl->Process, Mdl->MappedSystemVa, Mdl->ByteOffset);

    trace("(TLine=%d) Mdl=0x%p S=%hu P=0x%p M=0x%p\n",
          line, Mdl,
          (USHORT)Mdl->Size, Mdl->Process, Mdl->MappedSystemVa);
*/

    ok(Mdl->Next == NULL,
       "(TLine=%d) Mdl->Next (0x%p) != NULL\n",
       line, Mdl->Next);

    ok((USHORT)Mdl->Size == Size,
       "(TLine=%d) Mdl->Size (%hu) != %hu\n",
       line, (USHORT)Mdl->Size, Size);

    ok((USHORT)Mdl->MdlFlags == MdlFlags,
       "(TLine=%d) Mdl->MdlFlags (0x%04hx) != 0x%04hx\n",
       line, (USHORT)Mdl->MdlFlags, MdlFlags);

    // TODO: Check Mdl->Process?

    // TODO: Check Mdl->MappedSystemVa?

    ok(Mdl->StartVa == PAGE_ALIGN(VirtualAddress),
       "(TLine=%d) Mdl->StartVa (0x%p) != 0x%p\n",
       line, Mdl->StartVa, PAGE_ALIGN(VirtualAddress));

    ok(Mdl->ByteCount == Length,
       "(TLine=%d) Mdl->ByteCount (%lu) != %lu\n",
       line, Mdl->ByteCount, Length);
    // Test MmGetMdlByteCount() too.
    ok(MmGetMdlByteCount(Mdl) == MdlCopy.ByteCount,
       "(TLine=%d) MmGetMdlByteCount(Mdl) (%lu) != %lu\n",
       line, MmGetMdlByteCount(Mdl), MdlCopy.ByteCount);
    ok(memcmp(&MdlCopy, Mdl, sizeof(MdlCopy)) == 0,
       "(TLine=%d) Mdl content changed after MmGetMdlByteCount()\n",
       line);

    ok(Mdl->ByteOffset == BYTE_OFFSET(VirtualAddress),
       "(TLine=%d) Mdl->ByteOffset (%lu) != %lu\n",
       line, Mdl->ByteOffset, BYTE_OFFSET(VirtualAddress));
    // Test MmGetMdlByteOffset() too.
    ok(MmGetMdlByteOffset(Mdl) == MdlCopy.ByteOffset,
       "(TLine=%d) MmGetMdlByteOffset(Mdl) (%lu) != %lu\n",
       line, MmGetMdlByteOffset(Mdl), MdlCopy.ByteOffset);
    ok(memcmp(&MdlCopy, Mdl, sizeof(MdlCopy)) == 0,
       "(TLine=%d) Mdl content changed after MmGetMdlByteOffset()\n",
       line);

    // Test MmGetMdlVirtualAddress().
    ok(MmGetMdlVirtualAddress(Mdl) == VirtualAddress,
       "(TLine=%d) MmGetMdlVirtualAddress(Mdl) (0x%p) != 0x%p\n",
       line, MmGetMdlVirtualAddress(Mdl), VirtualAddress);
    ok(memcmp(&MdlCopy, Mdl, sizeof(MdlCopy)) == 0,
       "(TLine=%d) Mdl content changed after MmGetMdlVirtualAddress()\n",
       line);
}

/*
https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-ioallocatemdl
PMDL IoAllocateMdl(
  __drv_aliasesMem PVOID VirtualAddress,
  ULONG                  Length,
  BOOLEAN                SecondaryBuffer,
  BOOLEAN                ChargeQuota,
  PIRP                   Irp
);
*/

#define _1KIB 0x00000400
#define _1MIB 0x00100000
#define _1GIB 0x40000000
// As officially documented...
#define MAX_NT50_SIZE   (PAGE_SIZE * (0xFFFF - sizeof(MDL)    ) / sizeof(ULONG_PTR))
#ifdef ALEKSEY_TESTS
// Test maximum size for an MDL.
#define TooLargeMdlSize (PAGE_SIZE * (0xFFFF - sizeof(MDL) + 1) / sizeof(ULONG_PTR))
#endif

static
VOID
TestIoAllocateMdl_Addr(PVOID VirtualAddress)
{
    const USHORT OSize = BYTE_OFFSET(VirtualAddress) == 0 ? 0 : 4;
    const struct
    {
        // Test parameters.
        INT    Line;
        USHORT NtVersion;
        ULONG  Length;
        // Expected results.
        USHORT Size;
        USHORT MdlFlags;
    }
    Tests[] =
    {
        // NT5.0+.
        // Doc says max Length is 'MAX_NT50_SIZE',
        // though it seems to be 'MAX_NT50_SIZE - PAGE_SIZE + 1024' (= 63.96875 MiB.) actually.

        { __LINE__, 0x0500,   0,
             28 + OSize, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,   1,
             32, MDL_ALLOCATED_FIXED_SIZE },
#ifdef INTERMEDIATE_TESTS
        { __LINE__, 0x0500, PAGE_SIZE,
             32, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,   8UL * _1KIB,
             36, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,  16UL * _1KIB,
             44, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,  32UL * _1KIB,
             60, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,  64UL * _1KIB,
             92, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,  65UL * _1KIB,
             96, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,  66UL * _1KIB,
             96, MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0500,  67UL * _1KIB,
             96, MDL_ALLOCATED_FIXED_SIZE },
#endif
        { __LINE__, 0x0500,  68UL * _1KIB,
             96 + OSize, g_NtVersion < 0x0600 || OSize == 0 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  68UL * _1KIB + 1,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
#ifdef INTERMEDIATE_TESTS
        { __LINE__, 0x0500,  68UL * _1KIB + 256,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  68UL * _1KIB + 512,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  68UL * _1KIB + 768,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  69UL * _1KIB - 1,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  69UL * _1KIB,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  70UL * _1KIB,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  71UL * _1KIB,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  72UL * _1KIB,
            100, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  80UL * _1KIB,
            108, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  88UL * _1KIB,
            116, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  89UL * _1KIB,
            120, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  90UL * _1KIB,
            120, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  91UL * _1KIB,
            120, g_NtVersion < 0x0600 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
#endif
        { __LINE__, 0x0500,  92UL * _1KIB,
            120 + OSize, g_NtVersion < 0x0600 && OSize == 0 ? MDL_ALLOCATED_FIXED_SIZE : MDL_NONE },
        { __LINE__, 0x0500,  92UL * _1KIB + 1,
            124, MDL_NONE },
#ifdef INTERMEDIATE_TESTS
        { __LINE__, 0x0500,  92UL * _1KIB + 256,
            124, MDL_NONE },
        { __LINE__, 0x0500,  92UL * _1KIB + 512,
            124, MDL_NONE },
        { __LINE__, 0x0500,  92UL * _1KIB + 768,
            124, MDL_NONE },
        { __LINE__, 0x0500,  93UL * _1KIB - 1,
            124, MDL_NONE },
        { __LINE__, 0x0500,  93UL * _1KIB,
            124, MDL_NONE },
        { __LINE__, 0x0500,  94UL * _1KIB,
            124, MDL_NONE },
        { __LINE__, 0x0500,  95UL * _1KIB,
            124, MDL_NONE },
        { __LINE__, 0x0500,  96UL * _1KIB,
            124, MDL_NONE },
        { __LINE__, 0x0500, 112UL * _1KIB,
            140, MDL_NONE },
        { __LINE__, 0x0500, 128UL * _1KIB,
            156, MDL_NONE },
        { __LINE__, 0x0500, 256UL * _1KIB,
            284, MDL_NONE },
        { __LINE__, 0x0500, 512UL * _1KIB,
            540, MDL_NONE },
        { __LINE__, 0x0500,   1UL * _1MIB,
           1052, MDL_NONE },
        { __LINE__, 0x0500,   2UL * _1MIB,
           2076, MDL_NONE },
        { __LINE__, 0x0500,   3UL * _1MIB,
           3100, MDL_NONE },
        { __LINE__, 0x0500,   4UL * _1MIB,
           4124, MDL_NONE },
        { __LINE__, 0x0500,   5UL * _1MIB,
           5148, MDL_NONE },
        { __LINE__, 0x0500,   6UL * _1MIB,
           6172, MDL_NONE },
        { __LINE__, 0x0500,   7UL * _1MIB,
           7196, MDL_NONE },
        { __LINE__, 0x0500,   8UL * _1MIB,
           8220, MDL_NONE },
        { __LINE__, 0x0500,  16UL * _1MIB,
          16412, MDL_NONE },
        { __LINE__, 0x0500,  32UL * _1MIB,
          32796, MDL_NONE },
        { __LINE__, 0x0500,  63UL * _1MIB,
          64540, MDL_NONE },
        { __LINE__, 0x0500, MAX_NT50_SIZE - PAGE_SIZE,
          65532, MDL_NONE },
#endif
        { __LINE__, OSize == 0 ? 0x0500 : 0x0600, MAX_NT50_SIZE - PAGE_SIZE + 1024,
          OSize == 0 ? 65532 : 0, MDL_NONE },

#ifdef ALEKSEY_TESTS
        { __LINE__, 0x0500, TooLargeMdlSize / 2,
          32784, MDL_NONE },
        { __LINE__, 0x0500, TooLargeMdlSize - PAGE_SIZE,
          65532, MDL_NONE },
#endif

        // NT6.0+.
        // Doc says max Length is '(2 gigabytes - PAGE_SIZE)',
        // though it seems to be '2 GiB - 1' actually.

#ifdef ALEKSEY_TESTS
        { __LINE__, 0x0600, TooLargeMdlSize - PAGE_SIZE + 1,
              0, MDL_NONE },
        { __LINE__, 0x0600, TooLargeMdlSize,
              0, MDL_NONE },
#endif

        { __LINE__, 0x0600, MAX_NT50_SIZE - PAGE_SIZE + 1024 + 1,
              0, MDL_NONE },
#ifdef INTERMEDIATE_TESTS
        { __LINE__, 0x0600, MAX_NT50_SIZE - 1,
              0, MDL_NONE },
        { __LINE__, 0x0600, MAX_NT50_SIZE,
              0, MDL_NONE },
        { __LINE__, 0x0600, MAX_NT50_SIZE + 1,
              0, MDL_NONE },
        { __LINE__, 0x0600, MAX_NT50_SIZE + PAGE_SIZE,
              4, MDL_NONE },
        { __LINE__, 0x0600,   1UL * _1GIB,
             28, MDL_NONE },
        { __LINE__, 0x0600,   2UL * _1GIB - PAGE_SIZE,
             24, MDL_NONE },
#endif
        { __LINE__, 0x0600,   2UL * _1GIB - 1,
             28 + (BYTE_OFFSET(VirtualAddress) <= 1 ? 0 : 4), MDL_NONE },

        // NT6.1+.
        // Doc says max Length is '(4 gigabytes - PAGE_SIZE)',
        // though it seems to be '4 GiB - 1' actually.

        { __LINE__, 0x0601,   2UL * _1GIB,
             28 + OSize, MDL_NONE },
#ifdef INTERMEDIATE_TESTS
        { __LINE__, 0x0601,   2UL * _1GIB + 1,
             32, MDL_NONE },
        { __LINE__, 0x0601,   2UL * _1GIB + PAGE_SIZE,
             32, MDL_NONE },
        { __LINE__, 0x0601,   3UL * _1GIB,
             28, MDL_NONE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE,
             24, MDL_NONE },
#endif
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 1,
             24 + OSize, g_NtVersion < 0x0602 || OSize == 0 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 1 + 1,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
#ifdef INTERMEDIATE_TESTS
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 3,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 4,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 5,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 6,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 7,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 8,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 16,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 24,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 32,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 64,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 128,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 256,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 512,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 1024,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 1024 + 1,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 2048,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 2048 + 1,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 3072,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - PAGE_SIZE + 3072 + 1,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
        { __LINE__, 0x0601, MAXULONG - 1,
             28, g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE },
#endif
        { __LINE__, 0x0601, MAXULONG,
             28 + (BYTE_OFFSET(VirtualAddress) <= 1 ? 0 : 4), g_NtVersion < 0x0602 ? MDL_NONE : MDL_ALLOCATED_FIXED_SIZE }
    };

    PMDL Mdl;
    ULONG Length;
#if 0
    // 2 pages and some random value.
    ULONG MdlSize = 2 * PAGE_SIZE + 184;
    PIRP Irp;
#endif
    /* UINT undeclared !!? */ unsigned int i;

    trace("VirtualAddress = 0x%p\n", VirtualAddress);

    for (i = 0; i < _countof(Tests); ++i)
    {
        // TODO: 'SecondaryBuffer = TRUE': succeeds too :-|
//ReTest+VA        // Mdl = IoAllocateMdl(VirtualAddress, Tests[i].Length, TRUE, FALSE, NULL);

        // TODO: 'ChargeQuota = TRUE': succeeds too :-|
//ReTest+VA        // Mdl = IoAllocateMdl(VirtualAddress, Tests[i].Length, FALSE, TRUE, NULL);

        Mdl = IoAllocateMdl(VirtualAddress, Tests[i].Length, FALSE, FALSE, NULL);

        if (Tests[i].NtVersion <= g_NtVersion)
        {
            // trace("(TLine=%d) Length = %lu\n",
            //       Tests[i].Line, Tests[i].Length);
/*
            printf("printf (TLine=%d) Length = %lu\n",
                   Tests[i].Line, Tests[i].Length);
*/
            ok(Mdl != NULL,
               "(TLine=%d) Mdl allocation failed\n",
               Tests[i].Line);
        }
        else
        {
            ok(Mdl == NULL,
               "(TLine=%d) Mdl allocation succeeded unexpectedly\n",
               Tests[i].Line);
        }

        if (Mdl != NULL)
        {
            CheckMdl(Tests[i].Line, Mdl, VirtualAddress, Tests[i].Length,
                     Tests[i].Size, Tests[i].MdlFlags);

            IoFreeMdl(Mdl);
        }
    }

    // Try with an unaligned VirtualAddress.
    Length = 1234;
    Mdl = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, NULL);
    ok(Mdl != NULL, "Mdl allocation failed\n");
    if (Mdl != NULL)
    {
        CHECK_MDL(Mdl, VirtualAddress, Length,
                  32 + (BYTE_OFFSET(VirtualAddress) <= 0x0B2E ? 0 : 4), MDL_ALLOCATED_FIXED_SIZE);
        IoFreeMdl(Mdl);

        // Try to free twice.
        // Hangs on (all) Windows! Even crashes on Vista.
        // IoFreeMdl(Mdl);
    }

#if 0
    //
    // Allocate a buffer.
    //
    VirtualAddress = ExAllocatePoolWithTag(NonPagedPool, MdlSize, 'tsTK');
    ok(VirtualAddress != NULL, "Buffer allocation failed\n");
    if (skip(VirtualAddress != NULL, "No buffer\n"))
    {
        return;
    }

    trace("VirtualAddress = 0x%p\n", VirtualAddress);

    for (i = 0; i < _countof(Tests); ++i)
    {
        // TODO: 'SecondaryBuffer = TRUE': succeeds too :-|
//        // Mdl = IoAllocateMdl(VirtualAddress, Tests[i].Length, TRUE, FALSE, NULL);

        // TODO: 'ChargeQuota = TRUE': succeeds too :-|
//        // Mdl = IoAllocateMdl(VirtualAddress, Tests[i].Length, FALSE, TRUE, NULL);

        Mdl = IoAllocateMdl(VirtualAddress, Tests[i].Length, FALSE, FALSE, NULL);

        if (Tests[i].NtVersion <= g_NtVersion)
        {
            // trace("(TLine=%d) Length = %lu\n",
            //       Tests[i].Line, Tests[i].Length);
/*
            printf("printf (TLine=%d) Length = %lu\n",
                   Tests[i].Line, Tests[i].Length);
*/
            ok(Mdl != NULL,
               "(TLine=%d) Mdl allocation failed\n",
               Tests[i].Line);
        }
        else
        {
            ok(Mdl == NULL,
               "(TLine=%d) Mdl allocation succeeded unexpectedly\n",
               Tests[i].Line);
        }

        if (Mdl != NULL)
        {
            CheckMdl(Tests[i].Line, Mdl, VirtualAddress, Tests[i].Length,
                     Tests[i].Size, Tests[i].MdlFlags);

            IoFreeMdl(Mdl);
        }
    }

    // Now create a valid MDL.
    Length = MdlSize;
    Mdl = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, NULL);
    ok(Mdl != NULL, "Mdl allocation failed\n");
    if (Mdl != NULL)
    {
        CHECK_MDL(Mdl, VirtualAddress, Length, 40, MDL_ALLOCATED_FIXED_SIZE);
        IoFreeMdl(Mdl);

        // Try to free twice.
        // Hangs on (all) Windows! Even crashes on Vista.
        // IoFreeMdl(Mdl);
    }

    //
    // Allocate an Irp.
    //
    Irp = IoAllocateIrp(1, FALSE);
    ok(Irp != NULL, "IRP allocation failed\n");
    if (skip(Irp != NULL, "No IRP\n"))
    {
        goto NoIrp;
    }

    // Allocate now with pointer to an Irp.
    Length = MdlSize;

    trace("VirtualAddress = 0x%p\n", NULL);

    // Crashes (all) Windows!
    // Mdl = IoAllocateMdl(NULL, Length, TRUE, FALSE, Irp);
    //
    Mdl = IoAllocateMdl(NULL, Length, FALSE, FALSE, Irp);
    ok(Mdl != NULL, "Mdl allocation failed\n");
    if (Mdl != NULL)
    {
        CHECK_MDL(Mdl, NULL, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

        ok(Irp->MdlAddress == Mdl,
           "Irp->MdlAddress (0x%p) != 0x%p\n",
           Irp->MdlAddress, Mdl);
        if (!skip(Irp->MdlAddress == Mdl, "Wrong Irp->MdlAddress\n"))
        {
            PMDL Mdl2;

            // Check a case when 'SecondaryBuffer == FALSE'.
            Mdl2 = IoAllocateMdl(NULL, Length, FALSE, FALSE, Irp);
            ok(Mdl2 != NULL, "Mdl allocation failed\n");
            if (Mdl2 != NULL)
            {
                ok(Mdl2 != Mdl, "Mdl2 == Mdl\n");

                CHECK_MDL(Mdl2, NULL, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

                ok(Irp->MdlAddress == Mdl2,
                   "Irp->MdlAddress (0x%p) != 0x%p\n",
                   Irp->MdlAddress, Mdl2);
                if (Irp->MdlAddress == Mdl2)
                {
                    // trace("Mdl2 FALSE test fully succeeded\n");

                    Irp->MdlAddress = Mdl;
                }

                IoFreeMdl(Mdl2);
            }

            ok(Irp->MdlAddress == Mdl,
               "Irp->MdlAddress (0x%p) != 0x%p\n",
               Irp->MdlAddress, Mdl);
            if (skip(Irp->MdlAddress == Mdl, "Unexpected Irp->MdlAddress\n"))
            {
                goto WrongIrpMdl;
            }

            // Check a case when 'SecondaryBuffer == TRUE'.
            Mdl2 = IoAllocateMdl(NULL, Length, TRUE, FALSE, Irp);
            ok(Mdl2 != NULL, "Mdl allocation failed\n");
            if (Mdl2 != NULL)
            {
                ok(Mdl2 != Mdl, "Mdl2 == Mdl\n");

                CHECK_MDL(Mdl2, NULL, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

                ok(Irp->MdlAddress == Mdl,
                   "Irp->MdlAddress (0x%p) != 0x%p\n",
                   Irp->MdlAddress, Mdl);
                if (!skip(Irp->MdlAddress == Mdl, "Wrong Irp->MdlAddress\n"))
                {
                    ok(Irp->MdlAddress->Next == Mdl2,
                       "Irp->MdlAddress->Next (0x%p) != 0x%p\n",
                       Irp->MdlAddress->Next, Mdl2);
                    if (!skip(Irp->MdlAddress->Next == Mdl2, "Wrong Irp->MdlAddress->Next\n"))
                    {
                        PMDL Mdl3;

                        // Check a case when 'SecondaryBuffer == TRUE'.
                        Mdl3 = IoAllocateMdl(NULL, Length, TRUE, FALSE, Irp);
                        ok(Mdl3 != NULL, "Mdl allocation failed\n");
                        if (Mdl3 != NULL)
                        {
                            ok(Mdl3 != Mdl , "Mdl3 == Mdl\n");
                            ok(Mdl3 != Mdl2, "Mdl3 == Mdl2\n");

                            CHECK_MDL(Mdl3, NULL, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

                            ok(Irp->MdlAddress == Mdl,
                               "Irp->MdlAddress (0x%p) != 0x%p\n",
                               Irp->MdlAddress, Mdl);
                            if (!skip(Irp->MdlAddress == Mdl, "Wrong Irp->MdlAddress\n"))
                            {
                                ok(Irp->MdlAddress->Next == Mdl2,
                                   "Irp->MdlAddress->Next (0x%p) != 0x%p\n",
                                   Irp->MdlAddress->Next, Mdl2);
                                if (!skip(Irp->MdlAddress->Next == Mdl2, "Wrong Irp->MdlAddress->Next\n"))
                                {
                                    ok(Irp->MdlAddress->Next->Next == Mdl3,
                                       "Irp->MdlAddress->Next->Next (0x%p) != 0x%p\n",
                                       Irp->MdlAddress->Next->Next, Mdl3);

                                    if (Irp->MdlAddress->Next->Next == Mdl3)
                                    {
                                        // trace("Mdl3 TRUE test fully succeeded\n");

                                        Irp->MdlAddress->Next->Next = NULL;
                                    }
                                }
                            }

                            IoFreeMdl(Mdl3);
                        }

                        Irp->MdlAddress->Next = NULL;
                    }
                }

                IoFreeMdl(Mdl2);
            }

            Irp->MdlAddress = NULL;
        }

WrongIrpMdl:
        IoFreeMdl(Mdl);
    }

    trace("VirtualAddress = 0x%p\n", VirtualAddress);

    // Hangs on (all) Windows!
    // Mdl = IoAllocateMdl(VirtualAddress, Length, TRUE, FALSE, Irp);
    //
    Mdl = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, Irp);
    ok(Mdl != NULL, "Mdl allocation failed\n");
    if (Mdl != NULL)
    {
        CHECK_MDL(Mdl, VirtualAddress, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

        ok(Irp->MdlAddress == Mdl,
           "Irp->MdlAddress (0x%p) != 0x%p\n",
           Irp->MdlAddress, Mdl);
        if (!skip(Irp->MdlAddress == Mdl, "Wrong Irp->MdlAddress\n"))
        {
            PMDL Mdl2;

            // Check a case when 'SecondaryBuffer == FALSE'.
            Mdl2 = IoAllocateMdl(VirtualAddress, Length, FALSE, FALSE, Irp);
            ok(Mdl2 != NULL, "Mdl allocation failed\n");
            if (Mdl2 != NULL)
            {
                ok(Mdl2 != Mdl, "Mdl2 == Mdl\n");

                CHECK_MDL(Mdl2, VirtualAddress, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

                ok(Irp->MdlAddress == Mdl2,
                   "Irp->MdlAddress (0x%p) != 0x%p\n",
                   Irp->MdlAddress, Mdl2);
                if (Irp->MdlAddress == Mdl2)
                {
                    // trace("Mdl2 FALSE test fully succeeded\n");

                    Irp->MdlAddress = Mdl;
                }

                IoFreeMdl(Mdl2);
            }

            ok(Irp->MdlAddress == Mdl,
               "Irp->MdlAddress (0x%p) != 0x%p\n",
               Irp->MdlAddress, Mdl);
            if (skip(Irp->MdlAddress == Mdl, "Unexpected Irp->MdlAddress\n"))
            {
                goto WrongIrpMdlVA;
            }

            // Check a case when 'SecondaryBuffer == TRUE'.
            Mdl2 = IoAllocateMdl(VirtualAddress, Length, TRUE, FALSE, Irp);
            ok(Mdl2 != NULL, "Mdl allocation failed\n");
            if (Mdl2 != NULL)
            {
                ok(Mdl2 != Mdl, "Mdl2 == Mdl\n");

                CHECK_MDL(Mdl2, VirtualAddress, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

                ok(Irp->MdlAddress == Mdl,
                   "Irp->MdlAddress (0x%p) != 0x%p\n",
                   Irp->MdlAddress, Mdl);
                if (!skip(Irp->MdlAddress == Mdl, "Wrong Irp->MdlAddress\n"))
                {
                    ok(Irp->MdlAddress->Next == Mdl2,
                       "Irp->MdlAddress->Next (0x%p) != 0x%p\n",
                       Irp->MdlAddress->Next, Mdl2);
                    if (!skip(Irp->MdlAddress->Next == Mdl2, "Wrong Irp->MdlAddress->Next\n"))
                    {
                        PMDL Mdl3;

                        // Check a case when 'SecondaryBuffer == TRUE'.
                        Mdl3 = IoAllocateMdl(VirtualAddress, Length, TRUE, FALSE, Irp);
                        ok(Mdl3 != NULL, "Mdl allocation failed\n");
                        if (Mdl3 != NULL)
                        {
                            ok(Mdl3 != Mdl , "Mdl3 == Mdl\n");
                            ok(Mdl3 != Mdl2, "Mdl3 == Mdl2\n");

                            CHECK_MDL(Mdl3, VirtualAddress, Length, 40, MDL_ALLOCATED_FIXED_SIZE);

                            ok(Irp->MdlAddress == Mdl,
                               "Irp->MdlAddress (0x%p) != 0x%p\n",
                               Irp->MdlAddress, Mdl);
                            if (!skip(Irp->MdlAddress == Mdl, "Wrong Irp->MdlAddress\n"))
                            {
                                ok(Irp->MdlAddress->Next == Mdl2,
                                   "Irp->MdlAddress->Next (0x%p) != 0x%p\n",
                                   Irp->MdlAddress->Next, Mdl2);
                                if (!skip(Irp->MdlAddress->Next == Mdl2, "Wrong Irp->MdlAddress->Next\n"))
                                {
                                    ok(Irp->MdlAddress->Next->Next == Mdl3,
                                       "Irp->MdlAddress->Next->Next (0x%p) != 0x%p\n",
                                       Irp->MdlAddress->Next->Next, Mdl3);

                                    if (Irp->MdlAddress->Next->Next == Mdl3)
                                    {
                                        // trace("Mdl3 TRUE test fully succeeded\n");

                                        Irp->MdlAddress->Next->Next = NULL;
                                    }
                                }
                            }

                            IoFreeMdl(Mdl3);
                        }

                        Irp->MdlAddress->Next = NULL;
                    }
                }

                IoFreeMdl(Mdl2);
            }

            Irp->MdlAddress = NULL;
        }

        IoFreeMdl(Mdl);
    }

WrongIrpMdlVA:
    IoFreeIrp(Irp);

NoIrp:
    ExFreePoolWithTag(VirtualAddress, 'tsTK');
#endif
}

static
VOID
TestIoAllocateMdl(VOID)
{
    trace("T1a: VirtualAddress = 0x%p\n", NULL);
    TestIoAllocateMdl_Addr(NULL);
    trace("T1b: VirtualAddress = 0x%p\n", ULongToPtr(0x00000001));
    TestIoAllocateMdl_Addr(ULongToPtr(0x00000001));
    trace("T1c: VirtualAddress = 0x%p\n", ULongToPtr(0x00000002));
    TestIoAllocateMdl_Addr(ULongToPtr(0x00000002));

#ifdef INTERMEDIATE_TESTS
    trace("T2a: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FF000));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FF000));
#endif
    trace("T2b: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FF0F0));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FF0F0));
#ifdef INTERMEDIATE_TESTS
    trace("T2c: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FF800));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FF800));
    trace("T2d: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFB2C));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFB2C));
    trace("T2e: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFB2D));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFB2D));
#endif
    trace("T2f: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFB2E));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFB2E));
    // 0x0B2F = 4096 - 1234 + 1.
    trace("T2g: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFB2F));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFB2F));
#ifdef INTERMEDIATE_TESTS
    trace("T2h: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFC00));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFC00));
    trace("T2h: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFD00));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFD00));
    trace("T2h: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFE00));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFE00));
    trace("T2h: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFF00));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFF00));

    trace("T3a: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFFFD));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFFFD));
    trace("T3b: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFFFE));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFFFE));
#endif
    trace("T3c: VirtualAddress = 0x%p\n", ULongToPtr(0x0F0FFFFF));
    TestIoAllocateMdl_Addr(ULongToPtr(0x0F0FFFFF));

    trace("T3y: VirtualAddress = 0x%p\n", ULongToPtr(0xFFFFFFFE));
    TestIoAllocateMdl_Addr(ULongToPtr(0xFFFFFFFE));
    trace("T3z: VirtualAddress = 0x%p\n", ULongToPtr(0xFFFFFFFF));
    TestIoAllocateMdl_Addr(ULongToPtr(0xFFFFFFFF));
}

static
VOID
TestIoFreeMdl(VOID)
{
    // Crashes (all) Windows!
#if 0
    // Try to free a NULL MDL.
    IoFreeMdl(NULL);
#endif
}

START_TEST(IoMdl)
{
    RTL_OSVERSIONINFOW osvi;

    RtlZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    RtlGetVersion(&osvi);
    g_NtVersion = osvi.dwMajorVersion << 8 | osvi.dwMinorVersion;
    trace("g_NtVersion = 0x%04hx\n", g_NtVersion);

    TestIoAllocateMdl();
    TestIoFreeMdl();
}
