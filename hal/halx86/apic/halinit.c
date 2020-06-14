/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Initialize the APIC HAL
 * COPYRIGHT:   Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#include "apicp.h"
#include <smp.h>

#define NDEBUG
#include <debug.h>

// Untested here. #define EARLY_DEBUG

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

VOID
NTAPI
ApicInitializeLocalApic(ULONG Cpu);

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
HalpInitProcessor(
    IN ULONG ProcessorNumber,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
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

#ifdef CONFIG_SMP
    if (ProcessorNumber == 0)
    {
#endif
        DPRINT1("1a\n");
        HalpParseApicTables(LoaderBlock);
        DPRINT1("1b\n");
// Local debug only.
        DPRINT1("Aa\n");
        HalpPrintApicTables();
        DPRINT1("Ab\n");
#ifdef CONFIG_SMP
    }

    DPRINT1("2a\n");
    HalpSetupProcessorsTable(ProcessorNumber);
    DPRINT1("2a\n");
#endif

    /* Initialize the local APIC for this cpu */
    DPRINT1("3a\n");
    ApicInitializeLocalApic(ProcessorNumber);
    DPRINT1("3b\n");

    /* Initialize profiling data (but don't start it) */
    DPRINT1("4a\n");
    HalInitializeProfiling();
    DPRINT1("4b\n");

    /* Initialize the timer */
    //ApicInitializeTimer(ProcessorNumber);
}

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    DPRINT1("Using HAL: APIC %s %s\n",
            (HalpBuildType & PRCB_BUILD_UNIPROCESSOR) ? "UP" : "SMP",
            (HalpBuildType & PRCB_BUILD_DEBUG) ? "DBG" : "REL");

    DPRINT1("5a\n");
    HalpPrintApicTables();
    DPRINT1("5b\n");

    /* Enable clock interrupt handler */
    DPRINT1("6a\n");
    HalpEnableInterruptHandler(IDT_INTERNAL,
                               0,
                               APIC_CLOCK_VECTOR,
                               CLOCK2_LEVEL,
                               HalpClockInterrupt,
                               Latched);
    DPRINT1("6b\n");
}

VOID
HalpInitPhase1(VOID)
{
    /* Initialize DMA. NT does this in Phase 0 */
    DPRINT1("7a\n");
    HalpInitDma();
    DPRINT1("7b\n");
}

/* EOF */
