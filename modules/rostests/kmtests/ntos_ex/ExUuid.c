/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite UUIDs test
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 *                  Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

START_TEST(ExUuid)
{
    UUID Uuid;
    NTSTATUS Status;
    unsigned char V;

    Status = ExUuidCreate(&Uuid);
    // FIXME: Why does Test WHS report RPC_NT_UUID_LOCAL_ONLY instead? (ROSTESTS-359)
    ok(Status == STATUS_SUCCESS, "ExUuidCreate returned unexpected status: %lx\n", Status);

    V = (Uuid.Data3 & 0xF000) >> 12;
    ok(V == 1, "UUID version: expected 1, got %u\n", V);

    V = (Uuid.Data4[0] & 0xF0) >> 4;
    if ((V & 0b1000) == 0b0000)
    {
        V = 0;
    }
    else if ((V & 0b1100) == 0b1000)
    {
        V = 1;
    }
    else if ((V & 0b1110) == 0b1100)
    {
        V = 2;
    }
    else // if ((V & 0b1110) == 0b1110)
    {
        // Reserved. Fake a value.
        V = 255;
    }
    ok(V == 1, "UUID variant: expected 1, got %u\n", V);
}
