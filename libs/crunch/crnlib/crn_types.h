// File: crn_types.h
// See Copyright Notice and license at the end of inc/crnlib.h
#pragma once

namespace crnlib
{
   typedef unsigned char      uint8;
   typedef signed char        int8;
   typedef unsigned short     uint16;
   typedef signed short       int16;
   typedef unsigned int       uint32;
   typedef uint32             uint;
   typedef signed int         int32;

   typedef unsigned __int64   uint64;
   typedef signed __int64     int64;

   const uint8  UINT8_MIN  = 0;
#ifndef UINT8_MAX
   const uint8  UINT8_MAX  = 0xFFU;
#endif
   const uint16 UINT16_MIN = 0;
#ifndef UINT16_MAX
   const uint16 UINT16_MAX = 0xFFFFU;
#endif
   const uint32 UINT32_MIN = 0;
#ifndef UINT32_MAX
   const uint32 UINT32_MAX = 0xFFFFFFFFU;
#endif
   const uint64 UINT64_MIN = 0;
#ifndef UINT64_MAX
   const uint64 UINT64_MAX = 0xFFFFFFFFFFFFFFFFULL; //0xFFFFFFFFFFFFFFFFui64;
#endif
#ifndef INT8_MIN
   const int8  INT8_MIN  = -128;
#endif
#ifndef INT8_MAX
   const int8  INT8_MAX  = 127;
#endif
#ifndef INT16_MIN
   const int16 INT16_MIN = -32768;
#endif
#ifndef INT16_MAX
   const int16 INT16_MAX = 32767;
#endif
#ifndef INT32_MIN
   const int32 INT32_MIN = (-2147483647 - 1);
#endif
#ifndef INT32_MAX
   const int32 INT32_MAX = 2147483647;
#endif
#ifndef INT64_MIN
   const int64 INT64_MIN = (int64)0x8000000000000000ULL; //(-9223372036854775807i64 - 1);
#endif
#ifndef INT64_MAX
   const int64 INT64_MAX = (int64)0x7FFFFFFFFFFFFFFFULL; // 9223372036854775807i64;
#endif

#ifdef CRNLIB_PLATFORM_PC_X64
   typedef unsigned __int64 uint_ptr;
   typedef unsigned __int64 uint32_ptr;
   typedef signed __int64 signed_size_t;
   typedef uint64 ptr_bits_t;
#else
   typedef unsigned int uint_ptr;
   typedef unsigned int uint32_ptr;
   typedef signed int signed_size_t;
   typedef uint32 ptr_bits_t;
#endif

   enum eVarArg { cVarArg };
   enum eClear { cClear };
   enum eNoClamp { cNoClamp };
   enum { cInvalidIndex = -1 };

   const uint cIntBits = 32;

   struct empty_type { };

} // namespace crnlib
