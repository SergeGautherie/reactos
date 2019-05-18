
// #include <stdint.h>
// #include <stdlib.h>

#define FAST_FAIL_STACK_COOKIE_CHECK_FAILURE 2

/* Should be random :-/ */
#if UINT32_MAX == UINTPTR_MAX
#define STACK_CHK_GUARD 0xe2dee396
#else
#define STACK_CHK_GUARD 0x595e9fbd94fda766
#endif
// void * __stack_chk_guard = (void *)0xf00df00d;
void * __stack_chk_guard = (void *)STACK_CHK_GUARD;
// uintptr_t __stack_chk_guard = STACK_CHK_GUARD;
#undef STACK_CHK_GUARD

#if 0
void __stack_chk_guard_setup(void)
{
    unsigned char * p;
    p = (unsigned char *)&__stack_chk_guard; // *** Notice that this takes the address of __stack_chk_guard  ***

    /* If you have the ability to generate random numbers in your kernel then use them,
       otherwise for 32-bit code: */
    *p =  0x00000aff; // *** p is &__stack_chk_guard so *p writes to __stack_chk_guard rather than *__stack_chk_guard ***
}
#endif

// Unknown here // DECLSPEC_NORETURN
// Or? // __attribute((noreturn))
__declspec(noreturn)
void __stack_chk_fail(void)
{
    // Unknown here // __debugbreak();
    // Unknown here // ASSERTMSG("(SG) Stop in __stack_chk_fail()\n", FALSE);
    __asm__("int $3"); // == __debugbreak();

    /* Like __fastfail */
    __asm__("int $0x29" : : "c"(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE) : "memory");
    __builtin_unreachable();
}
