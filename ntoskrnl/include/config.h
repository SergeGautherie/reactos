#pragma once

// Enable Driver Verifier checks in ObReferenceObjectByHandle(). (CORE-10207)
#define ENABLE_DRIVER_VERIFIER_OBREFERENCEOBJECTBYHANDLE

// Enable Special Pool for everything, but modules which are not win32k.sys. (CORE-10380)
#define ENABLE_SPECIAL_POOL_DEFAULT_AND_WIN32K

// Enable global page support.
// #define _GLOBAL_PAGES_ARE_AWESOME_
