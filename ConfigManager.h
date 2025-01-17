#ifndef AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#define AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#include "editwing/editwing.h"
#include "OpenSaveDlg.h"
#include "kilib/registry.h"

void SetFontSize(LOGFONT *font, HDC hDC, int fsiz, int fx);

// アプリケーションメッセージ
#define GPM_MRUCHANGED WM_APP+0

//=========================================================================
//@{ @pkg Gp.Main //@}
//@{
//	設定の一元管理
//
//	SetDocTypeで切り替えると、文書タイプ依存の項目を内部で
//	適切に切り替えたり色々します。
//@}
//=========================================================================

class ConfigManager
{
public:

	ConfigManager();
	~ConfigManager();

	//@{ 指定した名前のファイル用の文書タイプをロード //@}
	int SetDocType( const ki::Path& fname );

	//@{ 指定した番号の文書タイプをロード //@}
	void SetDocTypeByMenu( int pos, HMENU m );

	//@{ 指定した名前の文書タイプをロード //@}
	bool SetDocTypeByName( const ki::String& nam );

	//@{ メニュー項目作成 //@}
	void SetDocTypeMenu( HMENU m, UINT idstart );

	//@{ メニュー項目のチェック修正 //@}
	void CheckMenu( HMENU m, int pos );

	//@{ 設定ダイアログ表示 //@}
	bool DoDialog( const ki::Window& parent );

public:

	//@{ Undo回数制限値 //@}
	int undoLimit() const;

	//@{ 文字数のカウント方法 //@}
	bool countByUnicode() const;

	//@{ 開く/保存ダイアログに出すフィルタの設定 //@}
	const ki::String& txtFileFilter() const;

	//@{ 文字数指定時の折り返し文字数 //@}
	short wrapWidth() const;

	//@{ Get smart warp flag //@}
	bool wrapSmart() const;

	//@{ 折り返し方法 //@}
	short wrapType() const;

	//@{ 行番号表示する？ //@}
	bool showLN() const;

	//@{ 表示色・フォントなど //@}
	const editwing::VConfig& vConfig() const;

	//@{ キーワードファイル名(フルパス) //@}
	ki::Path kwdFile() const;

	//@{ Grep用外部実行ファイル名 //@}
	const ki::Path& grepExe() const;

	//@{ Help用外部実行ファイル名 //@}
	const ki::Path& helpExe() const;

	//@{ 同じウインドウで開くモード //@}
	bool openSame() const;

	//@{ ステータスバー表示 //@}
	bool showStatusBar() const;
	void ShowStatusBarSwitch();

	//@{ 日付 //@}
	const ki::String& dateFormat() const;

public:
	//@{ 新規ファイルの文字コードindex //@}
	int GetNewfileCsi() const;

	//@{ 新規ファイルの改行コード //@}
	ki::lbcode GetNewfileLB() const;

public:
	//@{ [最近使ったファイル]へ追加 //@}
	bool AddMRU( const ki::Path& fname );

	//@{ [最近使ったファイル]メニューの構築 //@}
	int SetUpMRUMenu( HMENU m, UINT id );

	//@{ [最近使ったファイル]取得 //@}
	ki::Path GetMRU( int no ) const;

	//@{ 対応文字セットリスト取得 //@}
	CharSetList& GetCharSetList();

public:
	//@{ ウインドウ位置・サイズ復元処理 //@}
	int GetWndX() const;
	int GetWndY() const;
	int GetWndW() const;
	int GetWndH() const;
	bool GetWndM() const;
	void RememberWnd( const ki::Window* wnd );
	const RECT *PMargins() const;
	void SetPrintMargins(const RECT *rc);
	bool useQuickExit() const;
	bool useOldOpenSaveDlg() const;
	bool warnOnModified() const;

private:

	CharSetList charSets_;

	// 全体的な設定
	int        undoLimit_;
	ki::String txtFilter_;
	ki::Path   grepExe_;
	ki::Path   helpExe_;

	bool        sharedConfigMode_;
	bool        inichanged_; // keep track of save to ini.

	bool       openSame_;
	bool       countbyunicode_;
	bool       showStatusBar_;
	bool       rememberWindowSize_;
	bool       rememberWindowPlace_;
	bool       useQuickExit_;
	bool       useOldOpenSaveDlg_;
	bool       warnOnModified_;

	ki::String dateFormat_;
//	ki::String timeFormat_;
//	bool datePrior_;

	// ウインドウサイズ記憶
	bool wndM_; // maximized?
	int  wndX_, wndY_, wndW_, wndH_;

	// 文書タイプのリスト
	struct DocType
	{
		// 定義ファイル名など
		ki::String        name;
		ki::String        pattern;
		ki::String        kwdfile;
		ki::String        layfile;

		// 設定項目
		editwing::VConfig vc;
		short             wrapWidth;
		signed char       wrapType;
		bool              wrapSmart;
		bool              showLN;
		uchar             fontCS;
		uchar             fontQual;
		bool              loaded;
	};
	typedef ki::olist<DocType> DtList;

	DtList           dtList_;
	DtList::iterator curDt_;

	RECT rcPMargins_;

	// 最近使ったファイルのリスト
	int mrus_;
	ki::Path mru_[20];

	// 新規ファイル関係
	int        newfileCharset_;
	ki::String newfileDoctype_;
	ki::lbcode newfileLB_;

private:

	void LoadIni();
	void SaveIni();
	void LoadLayout( DocType* dt );
	bool MatchDocType( const unicode* fname, const unicode* pat );

private:

	friend struct ConfigDlg;
	NOCOPY(ConfigManager);
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline int ConfigManager::undoLimit() const
	{ return undoLimit_; }

inline const ki::String& ConfigManager::txtFileFilter() const
	{ return txtFilter_; }

inline short ConfigManager::wrapWidth() const
	{ return curDt_->wrapWidth; }

inline bool ConfigManager::wrapSmart() const
	{ return curDt_->wrapSmart; }

inline short ConfigManager::wrapType() const
	{ return curDt_->wrapType>0 ? wrapWidth() : curDt_->wrapType; }

inline bool ConfigManager::showLN() const
	{ return curDt_->showLN; }

inline const editwing::VConfig& ConfigManager::vConfig() const
	{ return curDt_->vc; }

inline ki::Path ConfigManager::kwdFile() const
	{
	ki::Path p(ki::Path::Exe);
	p += TEXT("type\\");
	p += curDt_->kwdfile;
	return p;
	}

inline const ki::Path& ConfigManager::grepExe() const
	{ return grepExe_; }

inline const ki::Path& ConfigManager::helpExe() const
	{ return helpExe_; }

inline bool ConfigManager::openSame() const
	{ return openSame_; }

inline bool ConfigManager::showStatusBar() const
	{ return showStatusBar_; }

inline void ConfigManager::ShowStatusBarSwitch()
	{ showStatusBar_ = !showStatusBar_; inichanged_=1; SaveIni(); }

inline bool ConfigManager::countByUnicode() const
	{ return countbyunicode_; }

inline CharSetList& ConfigManager::GetCharSetList()
	{ return charSets_; }

inline int ConfigManager::GetNewfileCsi() const
	{ return charSets_.findCsi( newfileCharset_ ); }

inline ki::lbcode ConfigManager::GetNewfileLB() const
	{ return newfileLB_; }

inline const ki::String& ConfigManager::dateFormat() const
	{ return dateFormat_; }

inline int ConfigManager::GetWndX() const
	{ return rememberWindowPlace_ ? wndX_ : CW_USEDEFAULT; }

inline int ConfigManager::GetWndY() const
	{ return rememberWindowPlace_ ? wndY_ : CW_USEDEFAULT; }

inline int ConfigManager::GetWndW() const
	{ return rememberWindowSize_ ? wndW_ : CW_USEDEFAULT; }

inline int ConfigManager::GetWndH() const
	{ return rememberWindowSize_ ? wndH_ : CW_USEDEFAULT; }

inline bool ConfigManager::GetWndM() const
	{ return rememberWindowSize_ & wndM_; }

inline const RECT *ConfigManager::PMargins() const
	{ return &rcPMargins_; }

inline void ConfigManager::SetPrintMargins(const RECT *rc)
	{ CopyRect(&rcPMargins_, rc); inichanged_=1; SaveIni(); }

inline bool ConfigManager::useQuickExit() const
	{ return useQuickExit_; }

inline bool ConfigManager::useOldOpenSaveDlg() const
	{ return useOldOpenSaveDlg_; }

inline bool ConfigManager::warnOnModified() const
	{ return warnOnModified_; }

//=========================================================================

#endif // __ccdoc__
#endif // AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
