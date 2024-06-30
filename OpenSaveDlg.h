#ifndef _GREENPAD_OPENSAVEDLG_H_
#define _GREENPAD_OPENSAVEDLG_H_
#include "kilib/ktlarray.h"
#include "kilib/kstring.h"
#include "rsrc/resource.h"



//========================================================================
//@{ @pkg Gp.Dlg //@}
//@{
//	利用可能文字コードリスト
//@}
//========================================================================

class CharSetList
{
public:

	struct CsInfo
	{
		int                 ID;
		int               type;
		const TCHAR*  longName;
		const TCHAR* shortName;
	};

public:

	CharSetList();
	const CsInfo& operator[](size_t i) const { return list_[i]; }
	ulong size() const { return list_.size(); }
	int defaultCs() const;
	ulong defaultCsi() const;
	ulong findCsi( int cs ) const;
	void EnrollCs( int _id, uint _num);
	ulong GetCSIfromNumStr( const TCHAR *buf ) const;
	static int GetCSIFromComboBox( HWND dlg, const CharSetList& csl, uint OpenSaveMask );

private:

	enum { SAVE=1, LOAD=2, BOTH=3 };
	ki::storage<CsInfo> list_;
};



//========================================================================
//@{
//	「ファイルを開く」ダイアログ
//
//	Windows共通のダイアログの下に、文字コードの選択欄を
//	付け加えたものを表示する。
//@}
//========================================================================

class OpenFileDlg
{
public:
	explicit OpenFileDlg( const CharSetList& csl, bool oldstyle );
	bool DoModal( HWND wnd, const TCHAR* filter, const TCHAR* fnm );

public:
	const TCHAR* filename() const;
	int csi() const;

public:
	static ki::aarr<TCHAR> ConnectWithNull( const TCHAR *lst[], size_t num );

private:
	const CharSetList& csl_;
	TCHAR filename_[MAX_PATH];
	int   csIndex_;
	bool  dlgEverOpened_;
	bool  oldstyleDlg_;

private:
	static OpenFileDlg* pThis; // マルチスレッド禁止！
	static UINT_PTR CALLBACK OfnHook( HWND, UINT, WPARAM, LPARAM );
};



//------------------------------------------------------------------------
#ifndef __ccdoc__

inline OpenFileDlg::OpenFileDlg( const CharSetList& csl, bool oldstyle )
	: csl_(csl), csIndex_(0), dlgEverOpened_ (false), oldstyleDlg_(oldstyle)
	{ ki::mem00( filename_, sizeof(filename_) ); }

inline const TCHAR* OpenFileDlg::filename() const
	{ return filename_; }

inline int OpenFileDlg::csi() const
	{ return csIndex_; }



#endif // __ccdoc__
//========================================================================
//@{
//	「ファイルを保存」ダイアログ
//
//	Windows共通のダイアログの下に、文字コードの選択欄と
//	改行コードの選択欄を付け加えたものを表示する。
//@}
//========================================================================

class SaveFileDlg
{
public:
	explicit SaveFileDlg( const CharSetList& csl, int cs, int lb, bool oldstyle );
	bool DoModal( HWND wnd, const TCHAR* filter, const TCHAR* fnm );

public:
	const TCHAR* filename() const;
	int csi() const;
	int lb() const;

public:
	static ki::aarr<TCHAR> ConnectWithNull( const TCHAR *lst[], size_t num );

private:
	const CharSetList& csl_;
	TCHAR filename_[MAX_PATH];
	int   csIndex_;
	int   lb_;
	bool  dlgEverOpened_;
	bool  oldstyleDlg_;

private:
	static SaveFileDlg* pThis; // マルチスレッド禁止！
	static UINT_PTR CALLBACK OfnHook( HWND, UINT, WPARAM, LPARAM );
};

//------------------------------------------------------------------------
#ifndef __ccdoc__

inline SaveFileDlg::SaveFileDlg( const CharSetList& csl, int cs, int lb, bool oldstyle )
	: csl_(csl), csIndex_(cs), lb_(lb), dlgEverOpened_(false), oldstyleDlg_(oldstyle)
	{ ki::mem00( filename_, sizeof(filename_) ); }

inline const TCHAR* SaveFileDlg::filename() const
	{ return filename_; }

inline int SaveFileDlg::csi() const
	{ return csIndex_; }

inline int SaveFileDlg::lb() const
	{ return lb_; }

inline ki::aarr<TCHAR> SaveFileDlg::ConnectWithNull( const TCHAR *lst[], size_t num )
	{ return OpenFileDlg::ConnectWithNull( lst, num ); }



#endif // __ccdoc__
//========================================================================
//@{
//	「開き直す」ダイアログ
//
//	文字コード選択欄表示
//@}
//========================================================================

class ReopenDlg A_FINAL: public ki::DlgImpl
{
public:
	ReopenDlg( const CharSetList& csl, int csi );
	int csi() const;

private:
	void on_init() override;
	bool on_ok() override;

private:
	const CharSetList& csl_;
	int   csIndex_;
};

//------------------------------------------------------------------------
#ifndef __ccdoc__

inline int ReopenDlg::csi() const
	{ return csIndex_; }



//========================================================================

#endif // __ccdoc__
#endif // _GREENPAD_OPENSAVEDLG_H_
