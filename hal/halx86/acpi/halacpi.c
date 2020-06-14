/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/acpi/halacpi.c
 * PURPOSE:         HAL ACPI Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <smp.h>

// #define NDEBUG
#include <debug.h>

#define EARLY_DEBUG

/* GLOBALS ********************************************************************/

#if defined(EARLY_DEBUG)

ULONG (*FrLdrDbgPrint)(_In_ _Printf_format_string_ PCSTR Format, ...);

#undef DbgPrint
#define DbgPrint FrLdrDbgPrint

#else

#if 0

#undef DbgPrint

#if defined(_MSC_VER)
#define DbgPrint __noop
#else
#define DbgPrint
#endif // _MSC_VER

#endif // 0

#endif // EARLY_DEBUG

// TODO: Why 2? 1 should be enough?
#define ACPI_TABLE_DEFAULT_PAGE_NUMBER 2
static LIST_ENTRY HalpAcpiTableCacheList;
static FAST_MUTEX HalpAcpiTableCacheLock;

static BOOLEAN HalpProcessedACPIPhase0;
static BOOLEAN HalpPhysicalMemoryMayAppearAbove4GB;

static FADT HalpFixedAcpiDescTable;
static PDEBUG_PORT_TABLE HalpDebugPortTable;
static PACPI_SRAT HalpAcpiSrat;
static PBOOT_TABLE HalpSimpleBootFlagTable;

static PHYSICAL_ADDRESS HalpMaxHotPlugMemoryAddress;
PHYSICAL_ADDRESS HalpLowStubPhysicalAddress;
static PHARDWARE_PTE HalpPteForFlush;
static PVOID HalpVirtAddrForFlush;
PVOID HalpLowStub;

static PACPI_BIOS_MULTI_NODE HalpAcpiMultiNode;

static LIST_ENTRY HalpAcpiTableMatchList;

static ULONG HalpInvalidAcpiTable;

#if 1
static ULONG HalpPicVectorRedirect[15] = {0, 1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15};
#else // TODO: Is that correct?
ULONG HalpPicVectorRedirect[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
#endif

/* This determines the HAL type */
BOOLEAN HalDisableFirmwareMapper = TRUE;
PWCHAR HalHardwareIdString = L"acpipic_up";
PWCHAR HalName = L"ACPI Compatible Eisa/Isa HAL";

/* PRIVATE FUNCTIONS **********************************************************/

PDESCRIPTION_HEADER
NTAPI
HalpAcpiGetCachedTable(IN ULONG Signature)
{
    PLIST_ENTRY ListHead, NextEntry;
    PACPI_CACHED_TABLE CachedTable;

    /* Loop cached tables */
    ListHead = &HalpAcpiTableCacheList;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the table */
        CachedTable = CONTAINING_RECORD(NextEntry, ACPI_CACHED_TABLE, Links);

        /* Compare signatures */
        if (CachedTable->Header.Signature == Signature) return &CachedTable->Header;

        /* Keep going */
        NextEntry = NextEntry->Flink;
    }

    /* Nothing found */
    return NULL;
}

VOID
NTAPI
HalpAcpiCacheTable(IN PDESCRIPTION_HEADER TableHeader)
{
    PACPI_CACHED_TABLE CachedTable;

    /* Get the cached table and link it */
    CachedTable = CONTAINING_RECORD(TableHeader, ACPI_CACHED_TABLE, Header);
    InsertTailList(&HalpAcpiTableCacheList, &CachedTable->Links);
}

PVOID
NTAPI
HalpAcpiCopyBiosTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                      IN PDESCRIPTION_HEADER TableHeader)
{
    ULONG Size;
    PFN_COUNT PageCount;
    PHYSICAL_ADDRESS PhysAddress;
    PACPI_CACHED_TABLE CachedTable;
    PDESCRIPTION_HEADER CopiedTable;

    /* Size we'll need for the cached table */
    Size = TableHeader->Length + FIELD_OFFSET(ACPI_CACHED_TABLE, Header);
    if (LoaderBlock)
    {
        /* Phase 0: Convert to pages and use the HAL heap */
        PageCount = BYTES_TO_PAGES(Size);
        PhysAddress.QuadPart = HalpAllocPhysicalMemory(LoaderBlock,
                                                       0x1000000,
                                                       PageCount,
                                                       FALSE);
        if (PhysAddress.QuadPart)
        {
            /* Map it */
            CachedTable = HalpMapPhysicalMemory64(PhysAddress, PageCount);
        }
        else
        {
            /* No memory, so nothing to map */
            CachedTable = NULL;
        }
    }
    else
    {
        /* Use Mm pool */
        CachedTable = ExAllocatePoolWithTag(NonPagedPool, Size, TAG_HAL);
    }

    /* Do we have the cached table? */
    if (CachedTable)
    {
        /* Copy the data */
        CopiedTable = &CachedTable->Header;
        RtlCopyMemory(CopiedTable, TableHeader, TableHeader->Length);
    }
    else
    {
        /* Nothing to return */
        CopiedTable = NULL;
    }

    /* Return the table */
    return CopiedTable;
}

PVOID
NTAPI
HalpAcpiGetTableFromBios(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN ULONG Signature)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    static PXSDT Xsdt;
    static PRSDT Rsdt;
    PFADT Fadt;
    PDESCRIPTION_HEADER Header = NULL;
    UCHAR CheckSum;
    static ULONG EntryCount;
    ULONG CurrentEntry;
    PUCHAR CurrentByte;
    PFN_COUNT PageCount;

// Local debug only.
    DPRINT("HalpAcpiGetTableFromBios(%p, %c%c%c%c)\n", LoaderBlock,
            Signature & 0x000000FF,
           (Signature & 0x0000FF00) >>  8,
           (Signature & 0x00FF0000) >> 16,
           (Signature & 0xFF000000) >> 24);

    /* Should not query the RSDT/XSDT by itself */
    if ((Signature == RSDT_SIGNATURE) || (Signature == XSDT_SIGNATURE)) return NULL;

    /* Special case request for DSDT, because the FADT points to it */
    if (Signature == DSDT_SIGNATURE)
    {
        /* Grab the FADT */
        Fadt = HalpAcpiGetTable(LoaderBlock, FADT_SIGNATURE);
        if (!Fadt)
        {
            /* Fail */
            DPRINT1("HalpAcpiGetTable() failed\n");
            return NULL;
        }

        /* Grab the DSDT address and assume a default size */
        PhysicalAddress.HighPart = 0;
        PhysicalAddress.LowPart = Fadt->dsdt;

        /* Map it */
        if (LoaderBlock)
        {
            /* Phase 0, use HAL heap */
            Header = HalpMapPhysicalMemory64(PhysicalAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
        }
        else
        {
            /* Phase 1, use Mm */
            Header = MmMapIoSpace(PhysicalAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE, 0);
        }

        /* Fail if we couldn't map it */
        if (!Header)
        {
            DPRINT1("Failed to map DSDT table\n");
            return NULL;
        }

        /* Validate the signature */
        if (Header->Signature != DSDT_SIGNATURE)
        {
            DPRINT1("Table %c%c%c%c at 0x%p is not DSDT\n",
                     Header->Signature & 0x000000FF,
                    (Header->Signature & 0x0000FF00) >>  8,
                    (Header->Signature & 0x00FF0000) >> 16,
                    (Header->Signature & 0xFF000000) >> 24,
                    Header);

            /* Unmap */
            if (LoaderBlock)
            {
                /* Using HAL heap */
                HalpUnmapVirtualAddress(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
            }
            else
            {
                /* Using Mm */
                MmUnmapIoSpace(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE);
            }

            /* Fail */
            return NULL;
        }
    }
    else
    {
        /* Cache RSDT/XSDT address */
        if (!Xsdt && !Rsdt)
        {
            /* To find tables, we need the new XSDT */
            Xsdt = HalpAcpiGetTable(LoaderBlock, XSDT_SIGNATURE);
            if (Xsdt)
            {
                /* Count physical addresses */
                EntryCount = (Xsdt->Header.Length - FIELD_OFFSET(XSDT, Tables)) / sizeof(*Xsdt->Tables);
            }
            else
            {
                /* Fall back to the old RSDT */
                Rsdt = HalpAcpiGetTable(LoaderBlock, RSDT_SIGNATURE);
#if 0 // Impossible case
                if (!Rsdt)
                {
                    DPRINT1("HalpAcpiGetTable() failed\n");
                    return NULL;
                }
#endif

                /* Count 32-bit addresses */
                EntryCount = (Rsdt->Header.Length - FIELD_OFFSET(RSDT, Tables)) / sizeof(*Rsdt->Tables);
            }

            /* Extra-safetyIs table empty? */
            if (EntryCount == 0)
            {
                /* Fail */
                DPRINT1("No tables in %s table\n", Xsdt ? "XSDT" : "RSDT");
                return NULL;
            }
        }

        /* Start at the beginning of the array and loop it */
        for (CurrentEntry = 0; CurrentEntry < EntryCount; CurrentEntry++)
        {
            /* Get table physical address */
            if (Xsdt)
            {
                /* Read the 64-bit physical address */
                PhysicalAddress = Xsdt->Tables[CurrentEntry];
            }
            else
            {
                /* Read the 32-bit physical address */
                PhysicalAddress.QuadPart = Rsdt->Tables[CurrentEntry];
            }

            /* Now map this table */
            if (!LoaderBlock)
            {
                /* Phase 1: Use HAL heap */
                Header = MmMapIoSpace(PhysicalAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE,
                                      MmNonCached);
            }
            else
            {
                /* Phase 0: Use Mm */
                Header = HalpMapPhysicalMemory64(PhysicalAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
            }

            /* Check if we mapped it */
            if (!Header)
            {
                DPRINT1("Failed to map table %lu/%lu\n", CurrentEntry + 1, EntryCount);
                return NULL;
            }

#if 1 // TODO, here and elsewhere
            if (Header->Length < sizeof(*Header))
            {
                DPRINT1("Length is too short: %p, %u\n", Header, Header->Length);

                /* Unmap the table */
                if (LoaderBlock)
                {
                    /* Using HAL heap */
                    HalpUnmapVirtualAddress(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
                }
                else
                {
                    /* Using Mm */
                    MmUnmapIoSpace(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE);
                }

                return NULL;
            }
#endif

            DPRINT("Found %c%c%c%c table at 0x%p\n",
                    Header->Signature & 0xFF,
                    (Header->Signature & 0xFF00) >> 8,
                    (Header->Signature & 0xFF0000) >> 16,
                    (Header->Signature & 0xFF000000) >> 24,
                    Header);

            /* Did we find it? */
            if (Header->Signature == Signature) break;

            /* Unmap the table */
            if (LoaderBlock)
            {
                /* Using HAL heap */
                HalpUnmapVirtualAddress(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
            }
            else
            {
                /* Using Mm */
                MmUnmapIoSpace(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE);
            }
        }

        /* Not found? */
        if (CurrentEntry == EntryCount)
        {
            DPRINT("There is no %c%c%c%c table\n",
                    Signature & 0x000000FF,
                   (Signature & 0x0000FF00) >>  8,
                   (Signature & 0x00FF0000) >> 16,
                   (Signature & 0xFF000000) >> 24);
            return NULL;
        }
    }

    /* Past this point, we assume something was found */
    ASSERT(Header);

    /* How many pages do we need? */
    PageCount = BYTES_TO_PAGES(Header->Length);
    if (PageCount != ACPI_TABLE_DEFAULT_PAGE_NUMBER)
    {
        /* We assumed two, but this is not the case, free the current mapping */
        if (LoaderBlock)
        {
            /* Using HAL heap */
            HalpUnmapVirtualAddress(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
        }
        else
        {
            /* Using Mm */
            MmUnmapIoSpace(Header, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE);
        }

        /* Now map this table using its correct size */
        if (!LoaderBlock)
        {
            /* Phase 1: Use HAL heap */
            Header = MmMapIoSpace(PhysicalAddress, PageCount << PAGE_SHIFT, MmNonCached);
        }
        else
        {
            /* Phase 0: Use Mm */
            Header = HalpMapPhysicalMemory64(PhysicalAddress, PageCount);
        }

        /* Fail if remap failed */
        if (!Header)
        {
            DPRINT1("Failed to remap %c%c%c%c table (PageCount: %lu)\n",
                     Signature & 0x000000FF,
                    (Signature & 0x0000FF00) >>  8,
                    (Signature & 0x00FF0000) >> 16,
                    (Signature & 0xFF000000) >> 24,
                    PageCount);
            return NULL;
        }
        else
        {
            DPRINT("Remapped %c%c%c%c table (PageCount: %lu)\n",
                    Signature & 0x000000FF,
                   (Signature & 0x0000FF00) >>  8,
                   (Signature & 0x00FF0000) >> 16,
                   (Signature & 0xFF000000) >> 24,
                   PageCount);
        }
    }
    else
    {
        DPRINT("Mapped %c%c%c%c table (PageCount: %lu)\n",
                Signature & 0x000000FF,
               (Signature & 0x0000FF00) >>  8,
               (Signature & 0x00FF0000) >> 16,
               (Signature & 0xFF000000) >> 24,
               PageCount);
    }

// FIXME: Comment/Code mismatch...
    /* All tables in ACPI 3.0 other than the FACP should have correct checksum */
    if ((Header->Signature != FADT_SIGNATURE) || (Header->Revision > 2))
    {
        /* Go to the end of the table */
        CheckSum = 0;
        CurrentByte = (PUCHAR)Header + Header->Length;
        while (CurrentByte-- != (PUCHAR)Header)
        {
            /* Add this byte */
            CheckSum += *CurrentByte;
        }

        /* The correct checksum is always 0, anything else is illegal */
        if (CheckSum)
        {
            HalpInvalidAcpiTable = Header->Signature;
            DPRINT1("Checksum failed on %c%c%c%c table\n",
                    (Signature & 0xFF),
                    (Signature & 0xFF00) >> 8,
                    (Signature & 0xFF0000) >> 16,
                    (Signature & 0xFF000000) >> 24);
        }
        else
        {
            DPRINT("Checksum succeeded on %c%c%c%c table\n",
                    Signature & 0x000000FF,
                   (Signature & 0x0000FF00) >>  8,
                   (Signature & 0x00FF0000) >> 16,
                   (Signature & 0xFF000000) >> 24);
        }
    }

    /* Return the table */
    return Header;
}

PVOID
NTAPI
HalpAcpiGetTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                 IN ULONG Signature)
{
    PFN_COUNT PageCount;
    PDESCRIPTION_HEADER TableAddress, BiosCopy;

    /* See if we have a cached table? */
    TableAddress = HalpAcpiGetCachedTable(Signature);
    if (!TableAddress)
    {
        /* No cache, search the BIOS */
        TableAddress = HalpAcpiGetTableFromBios(LoaderBlock, Signature);
        if (TableAddress)
        {
            /* Found it, copy it into our own memory */
            BiosCopy = HalpAcpiCopyBiosTable(LoaderBlock, TableAddress);

            /* Get the pages, and unmap the BIOS copy */
            PageCount = BYTES_TO_PAGES(TableAddress->Length);
            if (LoaderBlock)
            {
                /* Phase 0, use the HAL heap */
                HalpUnmapVirtualAddress(TableAddress, PageCount);
            }
            else
            {
                /* Phase 1, use Mm */
                MmUnmapIoSpace(TableAddress, PageCount << PAGE_SHIFT);
            }

            /* Cache the bios copy */
            TableAddress = BiosCopy;
            if (BiosCopy) HalpAcpiCacheTable(BiosCopy);
        }
    }

    /* Return the table */
    return TableAddress;
}

PVOID
NTAPI
HalAcpiGetTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                IN ULONG Signature)
{
    PDESCRIPTION_HEADER TableHeader;

    /* Is this phase0 */
    if (LoaderBlock)
    {
        /* Initialize the cache first */
        if (!NT_SUCCESS(HalpAcpiTableCacheInit(LoaderBlock))) return NULL;
    }
    else
    {
        /* Lock the cache */
        ExAcquireFastMutex(&HalpAcpiTableCacheLock);
    }

    /* Get the table */
    TableHeader = HalpAcpiGetTable(LoaderBlock, Signature);

    /* Release the lock in phase 1 */
    if (!LoaderBlock) ExReleaseFastMutex(&HalpAcpiTableCacheLock);

    /* Return the table */
    return TableHeader;
}

VOID
NTAPI
HalpNumaInitializeStaticConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_SRAT SratTable;

    /* Get the SRAT, bail out if it doesn't exist */
    SratTable = HalAcpiGetTable(LoaderBlock, SRAT_SIGNATURE);
    HalpAcpiSrat = SratTable;
    if (!SratTable) return;
}

VOID
NTAPI
HalpGetHotPlugMemoryInfo(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_SRAT SratTable;

    /* Get the SRAT, bail out if it doesn't exist */
    SratTable = HalAcpiGetTable(LoaderBlock, SRAT_SIGNATURE);
    HalpAcpiSrat = SratTable;
    if (!SratTable) return;
}

VOID
NTAPI
HalpDynamicSystemResourceConfiguration(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* For this HAL, it means to get hot plug memory information */
    HalpGetHotPlugMemoryInfo(LoaderBlock);
}

VOID
NTAPI
HalpAcpiDetectMachineSpecificActions(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                                     IN PFADT DescriptionTable)
{
    /* Does this HAL specify something? */
    if (HalpAcpiTableMatchList.Flink)
    {
        /* Great, but we don't support it */
        DPRINT1("WARNING: Your HAL has specific ACPI hacks to apply!\n");
    }
}

VOID
NTAPI
HalpInitBootTable(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PBOOT_TABLE BootTable;

    /* Get the boot table */
    BootTable = HalAcpiGetTable(LoaderBlock, BOOT_SIGNATURE);
    HalpSimpleBootFlagTable = BootTable;

    /* Validate it */
    if ((BootTable) &&
        (BootTable->Header.Length >= sizeof(BOOT_TABLE)) &&
        (BootTable->CMOSIndex >= 9))
    {
        DPRINT1("BOOT table found, but not supported\n");
    }
    else
    {
        /* Invalid or doesn't exist, ignore it */
        HalpSimpleBootFlagTable = 0;
    }

    /* Install the end of boot handler */
//    HalEndOfBoot = HalpEndOfBoot;
}

NTSTATUS
NTAPI
HalpAcpiFindRsdtPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                       OUT PACPI_BIOS_MULTI_NODE* AcpiMultiNode)
{
    PCONFIGURATION_COMPONENT_DATA ComponentEntry = NULL;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    PACPI_BIOS_MULTI_NODE NodeData;
    SIZE_T NodeLength;
    PFN_COUNT PageCount;
    PVOID MappedAddress;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Did we already do this once? */
    if (HalpAcpiMultiNode)
    {
        /* Return what we know */
        *AcpiMultiNode = HalpAcpiMultiNode;
        return STATUS_SUCCESS;
    }

    /* Assume failure */
    *AcpiMultiNode = NULL;

// TODO: This does not look like the documented ACPI way to search for RSDP, to get RSDT and possibly XSDT.
// FIXME: Can this find the XSDT (if it exists), or the RSDT only?

    /* Search for the multi function adapter key */
    do
    {
        /* Find next entry */
        ComponentEntry = KeFindConfigurationNextEntry(LoaderBlock->ConfigurationRoot,
                                                      AdapterClass,
                                                      MultiFunctionAdapter,
                                                      NULL,
                                                      &ComponentEntry);
    }
    /* Is it the ACPI BIOS entry? */
    while (ComponentEntry && _stricmp(ComponentEntry->ComponentEntry.Identifier, "ACPI BIOS"));

    /* Make sure we found it */
    if (!ComponentEntry)
    {
        DPRINT1("HalpAcpiFindRsdtPhase0: did NOT find RSDT/XSDT\n");
        return STATUS_NOT_FOUND;
    }

    /* The configuration data is a resource list, and the BIOS node follows */
    ResourceList = ComponentEntry->ConfigurationData;
    NodeData = (PACPI_BIOS_MULTI_NODE)(ResourceList + 1);

    /* How many E820 memory entries are there? */
// TODO: Do we care about the E820 entries? They seem unused currently.
    NodeLength = sizeof(ACPI_BIOS_MULTI_NODE) +
                 (NodeData->Count - 1) * sizeof(ACPI_E820_ENTRY);

    /* Convert to pages */
    PageCount = (PFN_COUNT)BYTES_TO_PAGES(NodeLength);

    /* Allocate the memory */
    PhysicalAddress.QuadPart = HalpAllocPhysicalMemory(LoaderBlock,
                                                       0x1000000,
                                                       PageCount,
                                                       FALSE);
    if (PhysicalAddress.QuadPart)
    {
        /* Map it if the allocation worked */
        MappedAddress = HalpMapPhysicalMemory64(PhysicalAddress, PageCount);
    }
    else
    {
        /* Otherwise we'll have to fail */
        MappedAddress = NULL;
    }

    /* Save the multi node, bail out if we didn't find it */
    if (!MappedAddress) return STATUS_INSUFFICIENT_RESOURCES;
    HalpAcpiMultiNode = MappedAddress;

    /* Copy the multi-node data */
    RtlCopyMemory(MappedAddress, NodeData, NodeLength);

    /* Return the data */
    *AcpiMultiNode = HalpAcpiMultiNode;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HalpAcpiTableCacheInit(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_BIOS_MULTI_NODE AcpiMultiNode;
    NTSTATUS Status = STATUS_SUCCESS;
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID MappedAddress;
    ULONG TableLength;
    PDESCRIPTION_HEADER _sdt; // Header part of either RSDT/XSDT
    PLOADER_PARAMETER_EXTENSION LoaderExtension;

    /* Only initialize once */
    if (HalpAcpiTableCacheList.Flink) return Status;

    /* Setup the lock and table */
    ExInitializeFastMutex(&HalpAcpiTableCacheLock);
    InitializeListHead(&HalpAcpiTableCacheList);

    /* Find the table address */
    Status = HalpAcpiFindRsdtPhase0(LoaderBlock, &AcpiMultiNode);
    if (!NT_SUCCESS(Status)) return Status;

    PhysicalAddress.QuadPart = AcpiMultiNode->RsdtAddress.QuadPart;

    /* Map the table, assuming a default size */
    if (LoaderBlock)
    {
        /* Phase0: Use HAL Heap */
        MappedAddress = HalpMapPhysicalMemory64(PhysicalAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
    }
    else
    {
        /* Use an I/O map */
        MappedAddress = MmMapIoSpace(PhysicalAddress, PAGE_SIZE * ACPI_TABLE_DEFAULT_PAGE_NUMBER,
                                     MmNonCached);
    }

    /* Fail if we couldn't map it */
    if (!MappedAddress)
    {
        /* Fail, no memory */
        DPRINT1("HAL: Failed to map RSDT/XSDT\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _sdt = MappedAddress;

    /* Validate the signature */
    if ((_sdt->Signature != XSDT_SIGNATURE) && (_sdt->Signature != RSDT_SIGNATURE))
    {
        /* Very bad: crash */
        HalDisplayString("Bad RSDT/XSDT pointer/signature\r\n");
        KeBugCheckEx(MISMATCHED_HAL, 4, __LINE__, 0, 0);
    }

    /* We assumed two pages -- do we need less or more? */
    TableLength = ADDRESS_AND_SIZE_TO_SPAN_PAGES(PhysicalAddress.LowPart, _sdt->Length);
    if (TableLength != ACPI_TABLE_DEFAULT_PAGE_NUMBER)
    {
        DPRINT("Remapping %c%c%c%c table (TableLength: %lu)\n",
                _sdt->Signature & 0x000000FF,
               (_sdt->Signature & 0x0000FF00) >>  8,
               (_sdt->Signature & 0x00FF0000) >> 16,
               (_sdt->Signature & 0xFF000000) >> 24,
               TableLength);

        /* Unmap the old table, remap the new one */
        if (LoaderBlock)
        {
            /* Phase 0, use HAL heap */
            HalpUnmapVirtualAddress(MappedAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER);
            MappedAddress = HalpMapPhysicalMemory64(PhysicalAddress, TableLength);
        }
        else
        {
            /* Phase 1, use Mm I/O space */
            MmUnmapIoSpace(MappedAddress, ACPI_TABLE_DEFAULT_PAGE_NUMBER * PAGE_SIZE);
            MappedAddress = MmMapIoSpace(PhysicalAddress,
                                         TableLength << PAGE_SHIFT,
                                         MmNonCached);
        }

        /* Get the remapped table */
        if (!MappedAddress)
        {
            /* Fail, no memory */
            DPRINT1("HAL: Couldn't remap RSDT/XSDT\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        _sdt = MappedAddress;
    }
    else
    {
        DPRINT("Mapped %c%c%c%c table (TableLength: %lu)\n",
                _sdt->Signature & 0x000000FF,
               (_sdt->Signature & 0x0000FF00) >>  8,
               (_sdt->Signature & 0x00FF0000) >> 16,
               (_sdt->Signature & 0xFF000000) >> 24,
               TableLength);
    }

    /* Make a local copy */
    _sdt = HalpAcpiCopyBiosTable(LoaderBlock, _sdt);
    if (!_sdt)
    {
        /* Fail, no memory */
        DPRINT1("HAL: Couldn't copy RSDT/XSDT\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get rid of the BIOS mapping */
    if (LoaderBlock)
    {
        /* Use HAL heap */
        HalpUnmapVirtualAddress(MappedAddress, TableLength);

        LoaderExtension = LoaderBlock->Extension;
    }
    else
    {
        /* Use Mm */
        MmUnmapIoSpace(MappedAddress, TableLength << PAGE_SHIFT);

        LoaderExtension = NULL;
    }

    /* Cache the table */
    HalpAcpiCacheTable(_sdt);

    /* Check for compatible loader block extension */
// 0x58? SIZEOF_THROUGH_FIELD(AcpiTableSize)?
    if (LoaderExtension && (LoaderExtension->Size >= 0x58))
    {
        /* Compatible loader: did it provide an ACPI table override? */
        if ((LoaderExtension->AcpiTable) && (LoaderExtension->AcpiTableSize))
        {
            /* Great, because we don't support it! */
            DPRINT1("ACPI Table Overrides ignored: UNIMPLEMENTED\n");
        }
    }

    /* Done */
    return Status;
}

VOID
NTAPI
HaliAcpiTimerInit(IN ULONG TimerPort,
                  IN ULONG TimerValExt)
{
    PAGED_CODE();

    /* Is this in the init phase? */
    if (!TimerPort)
    {
        /* Get the data from the FADT */
        TimerPort = HalpFixedAcpiDescTable.pm_tmr_blk_io_port;
        TimerValExt = HalpFixedAcpiDescTable.flags & ACPI_TMR_VAL_EXT;
        DPRINT1("ACPI Timer at: %lXh (EXT: %lu)\n", TimerPort, TimerValExt);
    }

    /* FIXME: Now proceed to the timer initialization */
    //HalaAcpiTimerInit(TimerPort, TimerValExt);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    PFADT Fadt;
    ULONG TableLength;
    PHYSICAL_ADDRESS PhysicalAddress;

    /* Only do this once */
    if (HalpProcessedACPIPhase0) return STATUS_SUCCESS;

#if defined(EARLY_DEBUG)
    if (LoaderBlock)
    {
        /* Define your own function or use the trick with FreeLoader */
        FrLdrDbgPrint = ((PLOADER_PARAMETER_BLOCK)LoaderBlock)->u.I386.CommonDataArea;
    }
#if 0 // Future
    else
    {
        /* Define your own function or use the trick with FreeLoader */
        FrLdrDbgPrint = DPRINT1;
    }
#endif
#endif

    /* Setup the ACPI table cache */
    Status = HalpAcpiTableCacheInit(LoaderBlock);
    if (!NT_SUCCESS(Status)) return Status;

    /* Grab the FADT */
    Fadt = HalAcpiGetTable(LoaderBlock, FADT_SIGNATURE);
    if (!Fadt)
    {
        /* Fail */
        DPRINT1("HAL: Didn't find the FACP\n");
        return STATUS_NOT_FOUND;
    }

// debug only
   DPRINT1("FADT table: %p, Length %lu\n", Fadt, Fadt->Header.Length);

// Proper (header) length check must be common in GetFromBios (pass expected length param) and return NULL if needed!
    /* Require at least ACPI v1.0 length */
    if (Fadt->Header.Length < RTL_SIZEOF_THROUGH_FIELD(FADT, flags))
    {
        DPRINT1("Length is too short: %p, %u\n", Fadt, Fadt->Header.Length);
        return STATUS_NOT_FOUND;
    }

// Should be common check too.
    /* Detect possible newer revision of this table */
    if (Fadt->Header.Length > sizeof(*Fadt))
    {
        DPRINT1("Length is longer than expected: %p, %u\n", Fadt, Fadt->Header.Length);
    }

    /* Support older revisions too */
    TableLength = min(Fadt->Header.Length, sizeof(*Fadt));

#if DBG
    /* Length increased on v1.5. minor_revision is available since v5.1 (v5.0 by compatibility) */
    if (Fadt->Header.Length >= RTL_SIZEOF_THROUGH_FIELD(FADT, minor_revision) &&
        Fadt->Header.Revision >= 5)
    {
        UINT8 Errata = (Fadt->minor_revision & 0xF0) >> 4;

        DPRINT1("FADT table: ACPI v%u.%u-%c\n",
            Fadt->Header.Revision,
            Fadt->minor_revision & 0x0F,
            Errata ? 'A' + (Errata - 1) : 0);
    }
    else
    {
        DPRINT1("FADT table: ACPI v%u\n", Fadt->Header.Revision);
    }
#endif

    /* Copy it in the HAL static buffer */
    RtlCopyMemory(&HalpFixedAcpiDescTable, Fadt, TableLength);

    /* Anything special this HAL needs to do? */
    HalpAcpiDetectMachineSpecificActions(LoaderBlock, &HalpFixedAcpiDescTable);

    /* Get the debug table for KD */
    HalpDebugPortTable = HalAcpiGetTable(LoaderBlock, DBGP_SIGNATURE);

    /* Initialize NUMA through the SRAT */
    HalpNumaInitializeStaticConfiguration(LoaderBlock);

    /* Initialize hotplug through the SRAT */
    HalpDynamicSystemResourceConfiguration(LoaderBlock);
    if (HalpAcpiSrat)
    {
        DPRINT1("Your machine has a SRAT, but NUMA/HotPlug are not supported!\n");
    }

    /* Can there be memory higher than 4GB? */
    if (HalpMaxHotPlugMemoryAddress.HighPart >= 1)
    {
        /* We'll need this for DMA later */
        HalpPhysicalMemoryMayAppearAbove4GB = TRUE;
    }

    /* Setup the ACPI timer */
    HaliAcpiTimerInit(0, 0);

    /* Do we have a low stub address yet? */
    if (!HalpLowStubPhysicalAddress.QuadPart)
    {
        /* Allocate it */
        HalpLowStubPhysicalAddress.QuadPart = HalpAllocPhysicalMemory(LoaderBlock,
                                                                      0x100000,
                                                                      HALP_LOW_STUB_SIZE_IN_PAGES,
                                                                      FALSE);
        if (HalpLowStubPhysicalAddress.QuadPart)
        {
            /* Map it */
            HalpLowStub = HalpMapPhysicalMemory64(HalpLowStubPhysicalAddress, HALP_LOW_STUB_SIZE_IN_PAGES);
        }
    }

    /* Grab a page for flushes */
    PhysicalAddress.QuadPart = 0x100000;
    HalpVirtAddrForFlush = HalpMapPhysicalMemory64(PhysicalAddress, 1);
    HalpPteForFlush = HalAddressToPte(HalpVirtAddrForFlush);

    /* Don't do this again */
    HalpProcessedACPIPhase0 = TRUE;

    /* Setup the boot table */
    HalpInitBootTable(LoaderBlock);

#if DBG
    /* Debugging code */
    {
        PLIST_ENTRY ListHead, NextEntry;
        PACPI_CACHED_TABLE CachedTable;

// TODO: Add a similar loop for all=available tables. Then/Before, rework cache to cache missing/header/full tables...
        DPRINT1("ACPI tables in our cache:");

        /* Loop cached tables */
        ListHead = &HalpAcpiTableCacheList;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the table */
            CachedTable = CONTAINING_RECORD(NextEntry, ACPI_CACHED_TABLE, Links);

            DbgPrint(" [%c%c%c%c r%u]",
                      CachedTable->Header.Signature & 0x000000FF,
                     (CachedTable->Header.Signature & 0x0000FF00) >>  8,
                     (CachedTable->Header.Signature & 0x00FF0000) >> 16,
                     (CachedTable->Header.Signature & 0xFF000000) >> 24,
                     CachedTable->Header.Revision);

            /* Keep going */
            NextEntry = NextEntry->Flink;
        }

        DbgPrint("\n");
    }
#endif

    /* Return success */
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitializePciBus(VOID)
{
    /* Setup the PCI stub support */
    HalpInitializePciStubs();

    /* Set the NMI crash flag */
    HalpGetNMICrashFlag();
}

VOID
NTAPI
HalpInitNonBusHandler(VOID)
{
    /* These should be written by the PCI driver later, but we give defaults */
    HalPciTranslateBusAddress = HalpTranslateBusAddress;
    HalPciAssignSlotResources = HalpAssignSlotResources;
    HalFindBusAddressTranslation = HalpFindBusAddressTranslation;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitBusHandlers(VOID)
{
    /* On ACPI, we only have a fake PCI bus to worry about */
    HalpInitNonBusHandler();
}

CODE_SEG("INIT")
VOID
NTAPI
HalpBuildAddressMap(VOID)
{
    /* ACPI is magic baby */
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
HalpGetDebugPortTable(VOID)
{
    return ((HalpDebugPortTable) &&
            (HalpDebugPortTable->BaseAddress.AddressSpaceID == 1));
}

CODE_SEG("INIT")
ULONG
NTAPI
HalpIs16BitPortDecodeSupported(VOID)
{
    /* All ACPI systems are at least "EISA" so they support this */
    return CM_RESOURCE_PORT_16_BIT_DECODE;
}

VOID
NTAPI
HalpAcpiDetectResourceListSize(OUT PULONG ListSize)
{
    PAGED_CODE();

    /* One element if there is a SCI */
    *ListSize = HalpFixedAcpiDescTable.sci_int_vector ? 1 : 0;
}

NTSTATUS
NTAPI
HalpBuildAcpiResourceList(IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceList)
{
    ULONG Interrupt;
    PAGED_CODE();
    ASSERT(ResourceList != NULL);

    /* Initialize the list */
    ResourceList->BusNumber = -1;
    ResourceList->AlternativeLists = 1;
    ResourceList->InterfaceType = PNPBus;
    ResourceList->List[0].Version = 1;
    ResourceList->List[0].Revision = 1;
    ResourceList->List[0].Count = 0;

    /* Is there a SCI? */
    if (HalpFixedAcpiDescTable.sci_int_vector)
    {
        /* Fill out the entry for it */
        ResourceList->List[0].Descriptors[0].Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        ResourceList->List[0].Descriptors[0].Type = CmResourceTypeInterrupt;
        ResourceList->List[0].Descriptors[0].ShareDisposition = CmResourceShareShared;

        /* Get the interrupt number */
        Interrupt = HalpPicVectorRedirect[HalpFixedAcpiDescTable.sci_int_vector];
        ResourceList->List[0].Descriptors[0].u.Interrupt.MinimumVector = Interrupt;
        ResourceList->List[0].Descriptors[0].u.Interrupt.MaximumVector = Interrupt;

        /* One more */
        ++ResourceList->List[0].Count;
    }

    /* All good */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HalpQueryAcpiResourceRequirements(OUT PIO_RESOURCE_REQUIREMENTS_LIST *Requirements)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
    ULONG Count, ListSize;
    NTSTATUS Status;

    PAGED_CODE();

    /* Get ACPI resources */
    HalpAcpiDetectResourceListSize(&Count);
    DPRINT("Resource count: %lu\n", Count);

    /* Compute size of the list and allocate it */
    ListSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List[0].Descriptors) +
               (Count * sizeof(IO_RESOURCE_DESCRIPTOR));
    DPRINT("Resource list size: %lu\n", ListSize);
    RequirementsList = ExAllocatePoolWithTag(PagedPool, ListSize, TAG_HAL);
    if (RequirementsList)
    {
        /* Initialize it */
        RtlZeroMemory(RequirementsList, ListSize);
        RequirementsList->ListSize = ListSize;

        /* Build it */
        Status = HalpBuildAcpiResourceList(RequirementsList);
        if (NT_SUCCESS(Status))
        {
            /* It worked, return it */
            *Requirements = RequirementsList;

            /* Validate the list */
            ASSERT(RequirementsList->List[0].Count == Count);
        }
        else
        {
            /* Fail */
            ExFreePoolWithTag(RequirementsList, TAG_HAL);
            Status = STATUS_NO_SUCH_DEVICE;
        }
    }
    else
    {
        /* Not enough memory */
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return the status */
    return Status;
}

/*
 * @implemented
 */
CODE_SEG("INIT")
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    INTERFACE_TYPE InterfaceType;
    UNICODE_STRING HalString;

    /* FIXME: Initialize DMA 64-bit support */

    /* FIXME: Initialize MCA bus */

    /* Initialize PCI bus. */
    HalpInitializePciBus();

    /* What kind of bus is this? */
    switch (HalpBusType)
    {
        /* ISA Machine */
        case MACHINE_TYPE_ISA:
            InterfaceType = Isa;
            break;

        /* EISA Machine */
        case MACHINE_TYPE_EISA:
            InterfaceType = Eisa;
            break;

        /* MCA Machine */
        case MACHINE_TYPE_MCA:
            InterfaceType = MicroChannel;
            break;

        /* Unknown */
        default:
            InterfaceType = Internal;
            break;
    }

    /* Build HAL usage */
    RtlInitUnicodeString(&HalString, HalName);
    HalpReportResourceUsage(&HalString, InterfaceType);

    /* Setup PCI debugging and Hibernation */
    HalpRegisterPciDebuggingDeviceInfo();
}

/* EOF */
