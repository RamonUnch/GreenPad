#ifndef _MLANG_H
#define _MLANG_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
#pragma pack(push,1)

#define MAX_SCRIPT_NAME   48

typedef enum tagMLCPF {
    MLDETECTF_MAILNEWS = 0x0001,
    MLDETECTF_BROWSER = 0x0002,
    MLDETECTF_VALID = 0x0004,
    MLDETECTF_VALID_NLS = 0x0008,
    MLDETECTF_PRESERVE_ORDER = 0x0010,
    MLDETECTF_PREFERRED_ONLY = 0x0020,
    MLDETECTF_FILTER_SPECIALCHAR = 0x0040
} MLCP;
typedef enum tagMLDETECTCP {
    MLDETECTCP_NONE = 0,
    MLDETECTCP_7BIT = 1,
    MLDETECTCP_8BIT = 2,
    MLDETECTCP_DBCS = 4,
    MLDETECTCP_HTML = 8
} MLDETECTCP;
typedef enum tagSCRIPTFONTCONTF {
    SCRIPTCONTF_FIXED_FONT = 0x00000001,
    SCRIPTCONTF_PROPORTIONAL_FONT = 0x00000002,
    SCRIPTCONTF_SCRIPT_USER = 0x00010000,
    SCRIPTCONTF_SCRIPT_HIDE = 0x00020000,
    SCRIPTCONTF_SCRIPT_SYSTEM = 0x00040000
} SCRIPTFONTCONTF;
typedef enum tagSCRIPTCONTF {
    sidDefault,
    sidMerge,
    sidAsciiSym,
    sidAsciiLatin,
    sidLatin,
    sidGreek,
    sidCyrillic,
    sidArmenian,
    sidHebrew,
    sidArabic,
    sidDevanagari,
    sidBengali,
    sidGurmukhi,
    sidGujarati,
    sidOriya,
    sidTamil,
    sidTelugu,
    sidKannada,
    sidMalayalam,
    sidThai,
    sidLao,
    sidTibetan,
    sidGeorgian,
    sidHangul,
    sidKana,
    sidBopomofo,
    sidHan,
    sidEthiopic,
    sidCanSyllabic,
    sidCherokee,
    sidYi,
    sidBraille,
    sidRunic,
    sidOgham,
    sidSinhala,
    sidSyriac,
    sidBurmese,
    sidKhmer,
    sidThaana,
    sidMongolian,
    sidLim,
    sidFEFirst = sidHangul,
    sidFELast = sidHan
} SCRIPTCONTF;

typedef struct tagDetectEncodingInfo {
    UINT nLangID;
    UINT nCodePage;
    INT nDocPercent;
    INT nConfidence;
} DetectEncodingInfo, *pDetectEncodingInfo;


typedef BYTE SCRIPT_ID;
typedef struct tagSCRIPTINFO {
    SCRIPT_ID ScriptId;
    UINT uiCodePage;
    WCHAR wszDescription[MAX_SCRIPT_NAME];
    WCHAR wszFixedWidthFont[MAX_MIMEFACE_NAME];
    WCHAR wszProportionalFont[MAX_MIMEFACE_NAME];
} SCRIPTINFO, *PSCRIPTINFO;
typedef struct tagUNICODERANGE {
  WCHAR wcFrom;
  WCHAR wcTo;
} UNICODERANGE;

typedef struct tagSCRIPTFONTINFO SCRIPTFONTINFO;

EXTERN_C const IID IID_IEnumCodePage;

EXTERN_C const IID IID_IEnumRfc1766;

EXTERN_C const IID IID_IEnumScript;
#undef INTERFACE
#define INTERFACE IEnumScript
DECLARE_INTERFACE_(IEnumScript,IUnknown)
{
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
        STDMETHOD_(ULONG,AddRef)(THIS) PURE;
        STDMETHOD_(ULONG,Release)(THIS) PURE;
        STDMETHOD(Clone)(THIS_ IEnumScript**) PURE;
        STDMETHOD(Next)(THIS_ ULONG,PSCRIPTINFO,ULONG*) PURE;
        STDMETHOD(Reset)(THIS) PURE;
        STDMETHOD(Skip)(THIS_ ULONG) PURE;
};
#undef INTERFACE

EXTERN_C const IID IID_IMLangFontLink2;
#define INTERFACE IMLangFontLink2
DECLARE_INTERFACE_(IMLangFontLink2,IMLangCodePages)
{
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
        STDMETHOD_(ULONG,AddRef)(THIS) PURE;
        STDMETHOD_(ULONG,Release)(THIS) PURE;
        STDMETHOD(GetCharCodePages)(THIS_ WCHAR,DWORD*) PURE;
        STDMETHOD(GetStrCodePages)(THIS_ const WCHAR *,long,DWORD,DWORD*,long*) PURE;
        STDMETHOD(CodePageToCodePages)(THIS_ UINT,DWORD*) PURE;
        STDMETHOD(CodePagesToCodePage)(THIS_ DWORD,UINT,UINT*) PURE;
        STDMETHOD(GetFontCodePages)(THIS_ HDC,HFONT,DWORD*) PURE;
        STDMETHOD(ReleaseFont)(THIS_ HFONT) PURE;
        STDMETHOD(ResetFontMapping)(THIS) PURE;
        STDMETHOD(MapFont)(THIS_ HDC,DWORD,WCHAR,HFONT*) PURE;
        STDMETHOD(GetFontUnicodeRange)(THIS_ HDC,UINT*,UNICODERANGE*) PURE;
        STDMETHOD(GetScriptFontInfo)(THIS_ SCRIPT_ID,DWORD,UINT*,SCRIPTFONTINFO*) PURE;
        STDMETHOD(CodePageToScriptID)(THIS) PURE;
};
#undef INTERFACE

EXTERN_C const CLSID CLSID_CMultiLanguage;

EXTERN_C const IID IID_IMultiLanguage2;
#define INTERFACE IMultiLanguage2
DECLARE_INTERFACE_(IMultiLanguage2,IUnknown)
{
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
        STDMETHOD_(ULONG,AddRef)(THIS) PURE;
        STDMETHOD_(ULONG,Release)(THIS) PURE;
        STDMETHOD(GetNumberOfCodePageInfo)(THIS_ UINT*) PURE;
        STDMETHOD(GetCodePageInfo)(THIS_ UINT,PMIMECPINFO) PURE;
        STDMETHOD(GetFamilyCodePage)(THIS_ UINT,UINT*) PURE;
        STDMETHOD(EnumCodePages)(THIS_ DWORD,IEnumCodePage**) PURE;
        STDMETHOD(GetCharsetInfo)(THIS_ BSTR,PMIMECSETINFO) PURE;
        STDMETHOD(IsConvertible)(THIS_ DWORD,DWORD) PURE;
        STDMETHOD(ConvertString)(THIS_ DWORD*,DWORD,DWORD,BYTE*,UINT*,BYTE*,UINT*) PURE;
        STDMETHOD(ConvertStringToUnicode)(THIS_ DWORD*,DWORD,CHAR*,UINT*,WCHAR*,UINT*) PURE;
        STDMETHOD(ConvertStringFromUnicode)(THIS_ DWORD*,DWORD,WCHAR*,UINT*,CHAR*,UINT*) PURE;
        STDMETHOD(ConvertStringReset)(THIS) PURE;
        STDMETHOD(GetRfc1766FromLcid)(THIS_ LCID,BSTR*) PURE;
        STDMETHOD(GetLcidFromRfc1766)(THIS_ LCID*,BSTR) PURE;
        STDMETHOD(EnumRfc1766)(THIS_ IEnumRfc1766**) PURE;
        STDMETHOD(GetRfc1766Info)(THIS_ LCID,PRFC1766INFO) PURE;
        STDMETHOD(CreateConvertCharset)(THIS_ UINT,UINT,DWORD,IMLangConvertCharset**) PURE;
        STDMETHOD(ConvertStringInIStream)(THIS_ DWORD*,DWORD,WCHAR*,DWORD,DWORD,IStream*,IStream*) PURE;
        STDMETHOD(ConvertStringToUnicodeEx)(THIS_ DWORD*,DWORD,CHAR*,UINT*,WCHAR*,UINT*,DWORD,WCHAR*) PURE;
        STDMETHOD(ConvertStringFromUnicodeEx)(THIS_ DWORD*,DWORD,WCHAR*,UINT*,CHAR*,UINT*,DWORD,WCHAR*) PURE;
        STDMETHOD(DetectCodepageInIStream)(THIS_ DWORD,DWORD,IStream*,DetectEncodingInfo*,INT*) PURE;
        STDMETHOD(DetectInputCodepage)(THIS_ DWORD,DWORD,CHAR*,INT*,DetectEncodingInfo*,INT*) PURE;
        STDMETHOD(ValidateCodePage)(THIS) PURE;
        STDMETHOD(GetCodePageDescription)(THIS_ UINT,LCID,LPWSTR,int) PURE;
        STDMETHOD(IsCodePageInstallable)(THIS) PURE;
        STDMETHOD(SetMimeDBSource)(THIS_ MIMECONTF) PURE;
        STDMETHOD(GetNumberOfScripts)(THIS_ UINT*) PURE;
        STDMETHOD(EnumScripts)(THIS_ DWORD,LANGID,IEnumScript**) PURE;
};
#undef INTERFACE


#pragma pack(pop)
#ifdef __cplusplus
}
#endif
#endif
