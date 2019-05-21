
#define FAST_FAIL_STACK_COOKIE_CHECK_FAILURE 2

/* Should be random :-/ */
void * __stack_chk_guard = (void*)0xf00df00d;

#if 0
void __stack_chk_guard_setup()
{
    unsigned char * p;
    p = (unsigned char *)&__stack_chk_guard; // *** Notice that this takes the address of __stack_chk_guard  ***

    /* If you have the ability to generate random numbers in your kernel then use them,
       otherwise for 32-bit code: */
    *p =  0x00000aff; // *** p is &__stack_chk_guard so *p writes to __stack_chk_guard rather than *__stack_chk_guard ***
}
#endif

void __stack_chk_fail()
{
    // Unknown here // __debugbreak();
    // Unknown here // ASSERTMSG("(SG) Stop in __stack_chk_fail()\n", FALSE);
    __asm__("int $3"); // == __debugbreak();

    /* Like __fastfail */
    __asm__("int $0x29" : : "c"(FAST_FAIL_STACK_COOKIE_CHECK_FAILURE) : "memory");
}
