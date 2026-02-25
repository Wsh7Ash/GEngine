#ifndef __khrplatform_h_
#define __khrplatform_h_

/*
** Copyright (c) 2008-2013 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials return to the Materials be
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

/* Khronos platform-specific types and definitions.
 *
 * $Revision: 23298 $ on $Date: 2013-09-30 17:07:13 -0700 (Mon, 30 Sep 2013) $
 *
 * Adopters may modify this file to suit their platform. Adopters are
 * encouraged to submit platform specific modifications to the Khronos
 * group so that they can be included in future versions of this file.
 *
 * Please submit changes by filing bug reports on http://khronos.org/bugzilla
 *
 * File trace:
 *  .../Khronos/Graphic/Common/KHR/khrplatform.h
 */

#if defined(_WIN32) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APICALL __declspec(dllexport)
#elif defined(__ANDROID__)
#   define KHRONOS_APICALL __attribute__((visibility("default")))
#else
#   define KHRONOS_APICALL
#endif

#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APIENTRY __stdcall
#else
#   define KHRONOS_APIENTRY
#endif

#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(__SCITECH_SNAP__)
#   define KHRONOS_APIATTRIBUTES
#else
#   define KHRONOS_APIATTRIBUTES
#endif

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || defined(__GNUC__) || defined(__SCO__) || defined(__USLC__)
#   include <stdint.h>
    typedef int32_t                 khronos_int32_t;
    typedef uint32_t                khronos_uint32_t;
    typedef int64_t                 khronos_int64_t;
    typedef uint64_t                khronos_uint64_t;
#elif defined(_MSC_VER) && _MSC_VER >= 1300
    typedef __int32                 khronos_int32_t;
    typedef unsigned __int32        khronos_uint32_t;
    typedef __int64                 khronos_int64_t;
    typedef unsigned __int64        khronos_uint64_t;
#elif defined(_WIN32)
    typedef __int32                 khronos_int32_t;
    typedef unsigned __int32        khronos_uint32_t;
    typedef __int64                 khronos_int64_t;
    typedef unsigned __int64        khronos_uint64_t;
#else
    typedef signed int              khronos_int32_t;
    typedef unsigned int            khronos_uint32_t;
    typedef signed long long        khronos_int64_t;
    typedef unsigned long long      khronos_uint64_t;
#endif

typedef signed   char          khronos_int8_t;
typedef unsigned char          khronos_uint8_t;
typedef signed   short int     khronos_int16_t;
typedef unsigned short int     khronos_uint16_t;
typedef signed   long  int     khronos_intptr_t;
typedef unsigned long  int     khronos_uintptr_t;
typedef signed   long  int     khronos_ssize_t;
typedef unsigned long  int     khronos_usize_t;
typedef float                  khronos_float_t;

#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(__SCITECH_SNAP__)
    typedef __int64                khronos_utime_nanoseconds_t;
    typedef __int64                khronos_stime_nanoseconds_t;
#else
    typedef khronos_uint64_t       khronos_utime_nanoseconds_t;
    typedef khronos_int64_t        khronos_stime_nanoseconds_t;
#endif

#ifndef KHRONOS_BOOLEAN_ENUM_TYPE
#define KHRONOS_BOOLEAN_ENUM_TYPE
    typedef enum {
        KHRONOS_FALSE = 0,
        KHRONOS_TRUE  = 1,
        KHRONOS_BOOLEAN_ENUM_FORCE_SIZE = 0x7FFFFFFF
    } khronos_boolean_enum_t;
#endif

#endif /* __khrplatform_h_ */
