

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


#ifndef __vbscript_h__
#define __vbscript_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IRegExp_FWD_DEFINED__
#define __IRegExp_FWD_DEFINED__
typedef interface IRegExp IRegExp;

#endif 	/* __IRegExp_FWD_DEFINED__ */


#ifndef __IMatch_FWD_DEFINED__
#define __IMatch_FWD_DEFINED__
typedef interface IMatch IMatch;

#endif 	/* __IMatch_FWD_DEFINED__ */


#ifndef __IMatchCollection_FWD_DEFINED__
#define __IMatchCollection_FWD_DEFINED__
typedef interface IMatchCollection IMatchCollection;

#endif 	/* __IMatchCollection_FWD_DEFINED__ */


#ifndef __IRegExp2_FWD_DEFINED__
#define __IRegExp2_FWD_DEFINED__
typedef interface IRegExp2 IRegExp2;

#endif 	/* __IRegExp2_FWD_DEFINED__ */


#ifndef __IMatch2_FWD_DEFINED__
#define __IMatch2_FWD_DEFINED__
typedef interface IMatch2 IMatch2;

#endif 	/* __IMatch2_FWD_DEFINED__ */


#ifndef __IMatchCollection2_FWD_DEFINED__
#define __IMatchCollection2_FWD_DEFINED__
typedef interface IMatchCollection2 IMatchCollection2;

#endif 	/* __IMatchCollection2_FWD_DEFINED__ */


#ifndef __ISubMatches_FWD_DEFINED__
#define __ISubMatches_FWD_DEFINED__
typedef interface ISubMatches ISubMatches;

#endif 	/* __ISubMatches_FWD_DEFINED__ */


#ifndef __RegExp_FWD_DEFINED__
#define __RegExp_FWD_DEFINED__

typedef class RegExp RegExp;

#endif 	/* __RegExp_FWD_DEFINED__ */


#ifndef __Match_FWD_DEFINED__
#define __Match_FWD_DEFINED__

typedef class Match Match;

#endif 	/* __Match_FWD_DEFINED__ */


#ifndef __MatchCollection_FWD_DEFINED__
#define __MatchCollection_FWD_DEFINED__

typedef class MatchCollection MatchCollection;

#endif 	/* __MatchCollection_FWD_DEFINED__ */


#ifndef __SubMatches_FWD_DEFINED__
#define __SubMatches_FWD_DEFINED__

typedef class SubMatches SubMatches;

#endif 	/* __SubMatches_FWD_DEFINED__ */


extern "C"{



#ifndef __VBScript_RegExp_55_LIBRARY_DEFINED__
#define __VBScript_RegExp_55_LIBRARY_DEFINED__

/* library VBScript_RegExp_55 */
/* [helpstring][version][uuid] */ 









EXTERN_C const IID LIBID_VBScript_RegExp_55;

#ifndef __IRegExp_INTERFACE_DEFINED__
#define __IRegExp_INTERFACE_DEFINED__

/* interface IRegExp */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IRegExp;

    MIDL_INTERFACE("3F4DACA0-160D-11D2-A8E9-00104B365C9F")
    IRegExp : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Pattern( 
            /* [retval][out] */ BSTR *pPattern) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Pattern( 
            /* [in] */ BSTR pPattern) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IgnoreCase( 
            /* [retval][out] */ VARIANT_BOOL *pIgnoreCase) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_IgnoreCase( 
            /* [in] */ VARIANT_BOOL pIgnoreCase) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Global( 
            /* [retval][out] */ VARIANT_BOOL *pGlobal) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Global( 
            /* [in] */ VARIANT_BOOL pGlobal) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ BSTR sourceString,
            /* [retval][out] */ IDispatch **ppMatches) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Test( 
            /* [in] */ BSTR sourceString,
            /* [retval][out] */ VARIANT_BOOL *pMatch) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Replace( 
            /* [in] */ BSTR sourceString,
            /* [in] */ BSTR replaceString,
            /* [retval][out] */ BSTR *pDestString) = 0;
        
    };




#endif 	/* __IRegExp_INTERFACE_DEFINED__ */


#ifndef __IMatch_INTERFACE_DEFINED__
#define __IMatch_INTERFACE_DEFINED__

/* interface IMatch */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IMatch;

    MIDL_INTERFACE("3F4DACA1-160D-11D2-A8E9-00104B365C9F")
    IMatch : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ BSTR *pValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FirstIndex( 
            /* [retval][out] */ long *pFirstIndex) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [retval][out] */ long *pLength) = 0;
        
    };




#endif 	/* __IMatch_INTERFACE_DEFINED__ */


#ifndef __IMatchCollection_INTERFACE_DEFINED__
#define __IMatchCollection_INTERFACE_DEFINED__

/* interface IMatchCollection */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IMatchCollection;

    MIDL_INTERFACE("3F4DACA2-160D-11D2-A8E9-00104B365C9F")
    IMatchCollection : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IDispatch **ppMatch) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pCount) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppEnum) = 0;
        
    };




#endif 	/* __IMatchCollection_INTERFACE_DEFINED__ */


#ifndef __IRegExp2_INTERFACE_DEFINED__
#define __IRegExp2_INTERFACE_DEFINED__

/* interface IRegExp2 */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IRegExp2;

    MIDL_INTERFACE("3F4DACB0-160D-11D2-A8E9-00104B365C9F")
    IRegExp2 : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Pattern( 
            /* [retval][out] */ BSTR *pPattern) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Pattern( 
            /* [in] */ BSTR pPattern) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IgnoreCase( 
            /* [retval][out] */ VARIANT_BOOL *pIgnoreCase) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_IgnoreCase( 
            /* [in] */ VARIANT_BOOL pIgnoreCase) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Global( 
            /* [retval][out] */ VARIANT_BOOL *pGlobal) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Global( 
            /* [in] */ VARIANT_BOOL pGlobal) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Multiline( 
            /* [retval][out] */ VARIANT_BOOL *pMultiline) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Multiline( 
            /* [in] */ VARIANT_BOOL pMultiline) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Execute( 
            /* [in] */ BSTR sourceString,
            /* [retval][out] */ IDispatch **ppMatches) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Test( 
            /* [in] */ BSTR sourceString,
            /* [retval][out] */ VARIANT_BOOL *pMatch) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Replace( 
            /* [in] */ BSTR sourceString,
            /* [in] */ VARIANT replaceVar,
            /* [retval][out] */ BSTR *pDestString) = 0;
        
    };




#endif 	/* __IRegExp2_INTERFACE_DEFINED__ */


#ifndef __IMatch2_INTERFACE_DEFINED__
#define __IMatch2_INTERFACE_DEFINED__

/* interface IMatch2 */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IMatch2;

    MIDL_INTERFACE("3F4DACB1-160D-11D2-A8E9-00104B365C9F")
    IMatch2 : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ BSTR *pValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FirstIndex( 
            /* [retval][out] */ long *pFirstIndex) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Length( 
            /* [retval][out] */ long *pLength) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SubMatches( 
            /* [retval][out] */ IDispatch **ppSubMatches) = 0;
        
    };




#endif 	/* __IMatch2_INTERFACE_DEFINED__ */


#ifndef __IMatchCollection2_INTERFACE_DEFINED__
#define __IMatchCollection2_INTERFACE_DEFINED__

/* interface IMatchCollection2 */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_IMatchCollection2;

    MIDL_INTERFACE("3F4DACB2-160D-11D2-A8E9-00104B365C9F")
    IMatchCollection2 : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ IDispatch **ppMatch) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pCount) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppEnum) = 0;
        
    };




#endif 	/* __IMatchCollection2_INTERFACE_DEFINED__ */


#ifndef __ISubMatches_INTERFACE_DEFINED__
#define __ISubMatches_INTERFACE_DEFINED__

/* interface ISubMatches */
/* [object][oleautomation][nonextensible][dual][hidden][uuid] */ 


EXTERN_C const IID IID_ISubMatches;

    MIDL_INTERFACE("3F4DACB3-160D-11D2-A8E9-00104B365C9F")
    ISubMatches : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long index,
            /* [retval][out] */ VARIANT *pSubMatch) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pCount) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppEnum) = 0;
        
    };




#endif 	/* __ISubMatches_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_RegExp;

class DECLSPEC_UUID("3F4DACA4-160D-11D2-A8E9-00104B365C9F")
RegExp;

EXTERN_C const CLSID CLSID_Match;

class DECLSPEC_UUID("3F4DACA5-160D-11D2-A8E9-00104B365C9F")
Match;

EXTERN_C const CLSID CLSID_MatchCollection;

class DECLSPEC_UUID("3F4DACA6-160D-11D2-A8E9-00104B365C9F")
MatchCollection;

EXTERN_C const CLSID CLSID_SubMatches;

class DECLSPEC_UUID("3F4DACC0-160D-11D2-A8E9-00104B365C9F")
SubMatches;
#endif /* __VBScript_RegExp_55_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

}

#endif
