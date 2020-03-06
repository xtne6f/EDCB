#include "stdafx.h"


/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.01.0620 */
/* @@MIDL_FILE_HEADING(  ) */


extern "C"{


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_ITaskbarList,0x56FDF342,0xFD6D,0x11d0,0x95,0x8A,0x00,0x60,0x97,0xC9,0xA0,0x90);


MIDL_DEFINE_GUID(IID, IID_ITaskbarList2,0x602D4995,0xB13A,0x429b,0xA6,0x6E,0x19,0x35,0xE4,0x4F,0x43,0x17);


MIDL_DEFINE_GUID(IID, IID_ITaskbarList3,0xea1afb91,0x9e28,0x4b86,0x90,0xe9,0x9e,0x9f,0x8a,0x5e,0xef,0xaf);


MIDL_DEFINE_GUID(IID, IID_ITaskbarList4,0xc43dc798,0x95d1,0x4bea,0x90,0x30,0xbb,0x99,0xe2,0x98,0x3a,0x1a);


MIDL_DEFINE_GUID(CLSID, CLSID_TaskbarList,0x56FDF344,0xFD6D,0x11d0,0x95,0x8A,0x00,0x60,0x97,0xC9,0xA0,0x90);

#undef MIDL_DEFINE_GUID

}
