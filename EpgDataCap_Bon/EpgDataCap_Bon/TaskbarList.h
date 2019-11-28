

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0620 */
/* @@MIDL_FILE_HEADING(  ) */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include <rpc.h>
#include <rpcndr.h>

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __TaskbarList_h__
#define __TaskbarList_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITaskbarList_FWD_DEFINED__
#define __ITaskbarList_FWD_DEFINED__
typedef interface ITaskbarList ITaskbarList;

#endif 	/* __ITaskbarList_FWD_DEFINED__ */


#ifndef __ITaskbarList2_FWD_DEFINED__
#define __ITaskbarList2_FWD_DEFINED__
typedef interface ITaskbarList2 ITaskbarList2;

#endif 	/* __ITaskbarList2_FWD_DEFINED__ */


#ifndef __ITaskbarList3_FWD_DEFINED__
#define __ITaskbarList3_FWD_DEFINED__
typedef interface ITaskbarList3 ITaskbarList3;

#endif 	/* __ITaskbarList3_FWD_DEFINED__ */


#ifndef __ITaskbarList4_FWD_DEFINED__
#define __ITaskbarList4_FWD_DEFINED__
typedef interface ITaskbarList4 ITaskbarList4;

#endif 	/* __ITaskbarList4_FWD_DEFINED__ */


extern "C"{



#ifndef __ITaskbarList_INTERFACE_DEFINED__
#define __ITaskbarList_INTERFACE_DEFINED__

/* interface ITaskbarList */
/* [object][uuid] */ 


EXTERN_C const IID IID_ITaskbarList;

    MIDL_INTERFACE("56FDF342-FD6D-11d0-958A-006097C9A090")
    ITaskbarList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE HrInit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTab( 
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTab( 
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ActivateTab( 
            /* [in] */ HWND hwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActiveAlt( 
            /* [in] */ HWND hwnd) = 0;
        
    };




#endif 	/* __ITaskbarList_INTERFACE_DEFINED__ */


#ifndef __ITaskbarList2_INTERFACE_DEFINED__
#define __ITaskbarList2_INTERFACE_DEFINED__

/* interface ITaskbarList2 */
/* [object][uuid] */ 


EXTERN_C const IID IID_ITaskbarList2;

    MIDL_INTERFACE("602D4995-B13A-429b-A66E-1935E44F4317")
    ITaskbarList2 : public ITaskbarList
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE MarkFullscreenWindow( 
            /* [in] */ HWND hwnd,
            /* [in] */ BOOL fFullscreen) = 0;
        
    };




#endif 	/* __ITaskbarList2_INTERFACE_DEFINED__ */


typedef /* [v1_enum] */ 
enum THUMBBUTTONFLAGS
    {
        THBF_ENABLED	= 0,
        THBF_DISABLED	= 0x1,
        THBF_DISMISSONCLICK	= 0x2,
        THBF_NOBACKGROUND	= 0x4,
        THBF_HIDDEN	= 0x8,
        THBF_NONINTERACTIVE	= 0x10
    } 	THUMBBUTTONFLAGS;

DEFINE_ENUM_FLAG_OPERATORS(THUMBBUTTONFLAGS)
typedef /* [v1_enum] */ 
enum THUMBBUTTONMASK
    {
        THB_BITMAP	= 0x1,
        THB_ICON	= 0x2,
        THB_TOOLTIP	= 0x4,
        THB_FLAGS	= 0x8
    } 	THUMBBUTTONMASK;

DEFINE_ENUM_FLAG_OPERATORS(THUMBBUTTONMASK)
#include <pshpack8.h>
typedef struct THUMBBUTTON
    {
    THUMBBUTTONMASK dwMask;
    UINT iId;
    UINT iBitmap;
    HICON hIcon;
    WCHAR szTip[ 260 ];
    THUMBBUTTONFLAGS dwFlags;
    } 	THUMBBUTTON;

typedef struct THUMBBUTTON *LPTHUMBBUTTON;

#include <poppack.h>
#define THBN_CLICKED        0x1800


#ifndef __ITaskbarList3_INTERFACE_DEFINED__
#define __ITaskbarList3_INTERFACE_DEFINED__

/* interface ITaskbarList3 */
/* [object][uuid] */ 

typedef /* [v1_enum] */ 
enum TBPFLAG
    {
        TBPF_NOPROGRESS	= 0,
        TBPF_INDETERMINATE	= 0x1,
        TBPF_NORMAL	= 0x2,
        TBPF_ERROR	= 0x4,
        TBPF_PAUSED	= 0x8
    } 	TBPFLAG;

DEFINE_ENUM_FLAG_OPERATORS(TBPFLAG)

EXTERN_C const IID IID_ITaskbarList3;

    MIDL_INTERFACE("ea1afb91-9e28-4b86-90e9-9e9f8a5eefaf")
    ITaskbarList3 : public ITaskbarList2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProgressValue( 
            /* [in] */ HWND hwnd,
            /* [in] */ ULONGLONG ullCompleted,
            /* [in] */ ULONGLONG ullTotal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProgressState( 
            /* [in] */ HWND hwnd,
            /* [in] */ TBPFLAG tbpFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RegisterTab( 
            /* [in] */ HWND hwndTab,
            /* [in] */ HWND hwndMDI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnregisterTab( 
            /* [in] */ HWND hwndTab) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTabOrder( 
            /* [in] */ HWND hwndTab,
            /* [in] */ HWND hwndInsertBefore) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTabActive( 
            /* [in] */ HWND hwndTab,
            /* [in] */ HWND hwndMDI,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThumbBarAddButtons( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT cButtons,
            /* [size_is][in] */ LPTHUMBBUTTON pButton) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT cButtons,
            /* [size_is][in] */ LPTHUMBBUTTON pButton) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ThumbBarSetImageList( 
            /* [in] */ HWND hwnd,
            /* [in] */ HIMAGELIST himl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOverlayIcon( 
            /* [in] */ HWND hwnd,
            /* [in] */ HICON hIcon,
            /* [string][unique][in] */ LPCWSTR pszDescription) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThumbnailTooltip( 
            /* [in] */ HWND hwnd,
            /* [string][unique][in] */ LPCWSTR pszTip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetThumbnailClip( 
            /* [in] */ HWND hwnd,
            /* [in] */ RECT *prcClip) = 0;
        
    };




#endif 	/* __ITaskbarList3_INTERFACE_DEFINED__ */


#ifndef __ITaskbarList4_INTERFACE_DEFINED__
#define __ITaskbarList4_INTERFACE_DEFINED__

/* interface ITaskbarList4 */
/* [object][uuid] */ 

typedef /* [v1_enum] */ 
enum STPFLAG
    {
        STPF_NONE	= 0,
        STPF_USEAPPTHUMBNAILALWAYS	= 0x1,
        STPF_USEAPPTHUMBNAILWHENACTIVE	= 0x2,
        STPF_USEAPPPEEKALWAYS	= 0x4,
        STPF_USEAPPPEEKWHENACTIVE	= 0x8
    } 	STPFLAG;

DEFINE_ENUM_FLAG_OPERATORS(STPFLAG)

EXTERN_C const IID IID_ITaskbarList4;

    MIDL_INTERFACE("c43dc798-95d1-4bea-9030-bb99e2983a1a")
    ITaskbarList4 : public ITaskbarList3
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetTabProperties( 
            /* [in] */ HWND hwndTab,
            /* [in] */ STPFLAG stpFlags) = 0;
        
    };




#endif 	/* __ITaskbarList4_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_TaskbarList;

class DECLSPEC_UUID("56FDF344-FD6D-11d0-958A-006097C9A090")
TaskbarList;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

}

#endif
