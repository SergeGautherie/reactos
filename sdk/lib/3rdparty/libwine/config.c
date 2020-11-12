/*
 * Configuration parameters shared between Wine server and clients
 *
 * Copyright 2002 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**/

// Win32

#if 1 && defined(WIN32)
#error WIN32 is defined (libwine/config)
#endif

#if 0 && defined(_WIN32)
#error _WIN32 is defined (libwine/config)
#endif

// Win64

#if 1 && defined(WIN64)
#error WIN64 is defined (libwine/config)
#endif

#if 1 && defined(_WIN64)
#error _WIN64 is defined (libwine/config)
#endif

// i386

#if 1 && defined(i386)
#error i386 is defined (libwine/config)
#endif

#if 1 && defined(__i386__)
#error __i386__ is defined (libwine/config)
#endif

#if 1 && defined(_X86_)
#error _X86_ is defined (libwine/config)
#endif

#if 1 && defined(_M_IX86)
#error _M_IX86 is defined (libwine/config)
#endif

// amd64

#if 1 && defined(__x86_64__)
#error __x86_64__ is defined (libwine/config)
#endif

#if 1 && defined(_AMD64_)
#error _AMD64_ is defined (libwine/config)
#endif

#if 1 && defined(_M_AMD64)
#error _M_AMD64 is defined (libwine/config)
#endif

// arm

#if 1 && defined(__arm__)
#error __arm__ is defined (libwine/config)
#endif

#if 1 && defined(_ARM_)
#error _ARM_ is defined (libwine/config)
#endif

#if 0 && defined(_M_ARM)
#error _M_ARM is defined (libwine/config)
#endif

// arm64

#if 1 && defined(__arm64__)
#error __arm64__ is defined (libwine/config)
#endif

#if 1 && defined(_ARM64_)
#error _ARM64_ is defined (libwine/config)
#endif

#if 1 && defined(_M_ARM64)
#error _M_ARM64 is defined (libwine/config)
#endif

#error no more are defined (libwine/config)

/**/

#include "wine/library.h"

/* retrieve the wine data dir */
const char *wine_get_data_dir(void)
{
    return NULL;
}

/* retrieve the wine build dir (if we are running from there) */
const char *wine_get_build_dir(void)
{
    return NULL;
}
