/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for loading/unloading user32.dll
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 *                  Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include <apitest.h>
#include <ndk/pstypes.h>
#include <ndk/rtlfuncs.h>

START_TEST(load)
{
    HMODULE hImm32, hUser32, hUser32_2;
    BOOL Ret;
    DWORD Error;
    PPEB Peb = NtCurrentPeb();

    /* Before init */
    SetLastError(0xdeadbeef);
    hUser32 = GetModuleHandleW(L"user32");
    Error = GetLastError();

    if (hUser32 != NULL)
    {
        ok(hUser32 == NULL, "hUser32 = %p\n", hUser32);

        /* Check imm32, which could explain why user32 would still be loaded */
        SetLastError(0xdeadbeef);
        hImm32 = GetModuleHandleW(L"imm32");
        ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(hImm32);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(hImm32);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        Ret = FreeLibrary(hUser32);
//        Ret = FreeLibrary(???);
        ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

        SetLastError(0xdeadbeef);
        hUser32 = GetModuleHandleW(L"user32");
        Error = GetLastError();

        ok(hUser32 == NULL, "hUser32 = %p\n", hUser32);

        win_skip("user32.dll is already loaded\n");
        return;

//        ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    }
    else
    {
        ok(Peb->KernelCallbackTable == NULL, "KernelCallbackTable = %p\n", Peb->KernelCallbackTable);
    }

    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would already be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    Error = GetLastError();
    ok(hImm32 == NULL, "hImm32 = %p\n", hImm32);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);

    /* 1st load */
    SetLastError(0xdeadbeef);
    hUser32 = LoadLibraryW(L"user32");
    ok(hUser32 != NULL, "LoadLibrary failed: %lu\n", GetLastError());

    /* Initialized */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "GetModuleHandle failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

    /* 2nd load */
    SetLastError(0xdeadbeef);
    hUser32_2 = LoadLibraryW(L"user32");
    ok(hUser32_2 == hUser32, "LoadLibrary failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());

    /* Still initialized */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "GetModuleHandle failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

    /* Free to match 2nd load */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    /* Still initialized */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "GetModuleHandle failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

    /* Free to match 1st load */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    /* Somebody kept an extra reference! */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "GetModuleHandle failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

    /* Free to match extra reference */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(hImm32);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

/*
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());
*/

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    Error = GetLastError();
    ok(hImm32 == NULL, "hImm32 = %p\n", hImm32);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);

    /* Uninitialized */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    Error = GetLastError();
    ok(hUser32_2 == NULL, "hUser32_2 = %p\n", hUser32_2);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    /* Double-check */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == FALSE, "FreeLibrary returned %d\n", Ret);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    Error = GetLastError();
    ok(hImm32 == NULL, "hImm32 = %p\n", hImm32);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);

    /* Single load */
    SetLastError(0xdeadbeef);
    hUser32 = LoadLibraryW(L"user32");
    ok(hUser32 != NULL, "LoadLibrary failed: %lu\n", GetLastError());

    /* Initialized again */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "GetModuleHandle failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

    /* Free to match single load */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    /* Extra reference again */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    ok(hUser32_2 == hUser32, "GetModuleHandle failed: %p != %p, %lu\n", hUser32_2, hUser32, GetLastError());
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    ok(hImm32 != NULL, "GetModuleHandle failed: %lu\n", GetLastError());

    /* Free to match extra reference again */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(hImm32);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

/*
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());

    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
//    Ret = FreeLibrary(???);
    ok(Ret == TRUE, "FreeLibrary failed: %d, %lu\n", Ret, GetLastError());
*/

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    Error = GetLastError();
    ok(hImm32 == NULL, "hImm32 = %p\n", hImm32);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);

    /* Uninitialized again */
    SetLastError(0xdeadbeef);
    hUser32_2 = GetModuleHandleW(L"user32");
    Error = GetLastError();
    ok(hUser32_2 == NULL, "hUser32_2 = %p\n", hUser32_2);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);
    ok(Peb->KernelCallbackTable != NULL, "KernelCallbackTable is NULL\n");
    ok(Peb->PostProcessInitRoutine == NULL, "PostProcessInitRoutine = %p\n", Peb->PostProcessInitRoutine);
    /* Double-check */
    SetLastError(0xdeadbeef);
    Ret = FreeLibrary(hUser32);
    Error = GetLastError();
    ok(Ret == FALSE, "FreeLibrary returned %d\n", Ret);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);

    /* Check imm32, which could explain why user32 would still be loaded */
    SetLastError(0xdeadbeef);
    hImm32 = GetModuleHandleW(L"imm32");
    Error = GetLastError();
    ok(hImm32 == NULL, "hImm32 = %p\n", hImm32);
    ok(Error == ERROR_MOD_NOT_FOUND, "Error = %lu\n", Error);
}
