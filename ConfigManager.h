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

	ConfigManager() A_COLD;
	~ConfigManager() A_COLD;

	//@{ 指定した名前のファイル用の文書タイプをロード //@}
	int SetDocType( const ki::Path& fname )  A_COLD;

	//@{ 指定した番号の文書タイプをロード //@}
	void SetDocTypeByMenu( int pos, HMENU m )  A_COLD;

	//@{ 指定した名前の文書タイプをロード //@}
	bool SetDocTypeByName( const ki::String& nam )  A_COLD;

	//@{ メニュー項目作成 //@}
	void SetDocTypeMenu( HMENU m, UINT idstart )  A_COLD;

	//@{ メニュー項目のチェック修正 //@}
	void CheckMenu( HMENU m, int pos )  A_COLD;

	//@{ 設定ダイアログ表示 //@}
	bool DoDialog( const ki::Window& parent )  A_COLD;

public:

	//@{ Undo回数制限値 //@}
	inline int undoLimit() const { return undoLimit_; }

	//@{ 文字数のカウント方法 //@}
	inline bool countByUnicode() const { return countbyunicode_; }

	//@{ 開く/保存ダイアログに出すフィルタの設定 //@}
	inline const ki::String& txtFileFilter() const { return txtFilter_; }

	//@{ 文字数指定時の折り返し文字数 //@}
	inline short wrapWidth() const { return curDt_->wrapWidth; }

	//@{ Get smart warp flag //@}
	inline bool wrapSmart() const { return curDt_->wrapSmart; }

	//@{ 折り返し方法 //@}
	inline short wrapType() const { return curDt_->wrapType>0 ? wrapWidth() : curDt_->wrapType; }

	//@{ 行番号表示する？ //@}
	inline bool showLN() const { return curDt_->showLN; }

	//@{ 表示色・フォントなど //@}
	inline const editwing::VConfig& vConfig() const { return curDt_->vc; }

	//@{ キーワードファイル名(フルパス) //@}
	inline ki::Path kwdFile() const
		{
		ki::Path p(ki::Path::Exe);
		p += TEXT("type\\");
		p += curDt_->kwdfile;
		return p;
		}

	//@{ Grep用外部実行ファイル名 //@}
	inline const ki::Path& grepExe() const { return grepExe_; }

	//@{ Help用外部実行ファイル名 //@}
	inline const ki::Path& helpExe() const { return helpExe_; }

	//@{ 同じウインドウで開くモード //@}
	inline bool openSame() const { return openSame_; }

	//@{ ステータスバー表示 //@}
	inline bool showStatusBar() const { return showStatusBar_; }
	inline void ShowStatusBarSwitch() { showStatusBar_ = !showStatusBar_; inichanged_=1; SaveIni(); }

	//@{ 日付 //@}
	inline const ki::String& dateFormat() const { return dateFormat_; }

public:
	//@{ 新規ファイルの文字コードindex //@}
	inline int GetNewfileCsi() const { return charSets_.findCsi( newfileCharset_ ); }

	//@{ 新規ファイルの改行コード //@}
	inline ki::lbcode GetNewfileLB() const { return newfileLB_; }

public:
	//@{ [最近使ったファイル]へ追加 //@}
	bool AddMRU( const ki::Path& fname )  A_COLD;

	//@{ [最近使ったファイル]メニューの構築 //@}
	int SetUpMRUMenu( HMENU m, UINT id )  A_COLD;

	//@{ [最近使ったファイル]取得 //@}
	ki::Path GetMRU( int no ) const A_COLD;

	//@{ 対応文字セットリスト取得 //@}
	inline CharSetList& GetCharSetList() { return charSets_; }

public:
	//@{ ウインドウ位置・サイズ復元処理 //@}
	inline int GetWndX() const { return rememberWindowPlace_ ? wndPos_.left : CW_USEDEFAULT; }
	inline int GetWndY() const { return rememberWindowPlace_ ? wndPos_.top : CW_USEDEFAULT; }
	inline int GetWndW() const { return rememberWindowSize_ ? wndPos_.right-wndPos_.left : CW_USEDEFAULT; }
	inline int GetWndH() const { return rememberWindowSize_ ? wndPos_.bottom-wndPos_.top : CW_USEDEFAULT; }
	inline bool GetWndM() const { return rememberWindowSize_ && wndM_; }
	void RememberWnd( const ki::Window* wnd );
	inline const RECT *PMargins() const { return &rcPMargins_; }
	inline void SetPrintMargins(const RECT *rc) { CopyRect(&rcPMargins_, rc); inichanged_=1; SaveIni(); }
	inline bool useQuickExit() const { return useQuickExit_; }
	inline bool useOldOpenSaveDlg() const { return useOldOpenSaveDlg_; }
	inline bool warnOnModified() const { return warnOnModified_; }
	inline short GetZoom() const { return zoom_; };
	inline void SetZoom(short zoom) { zoom_ = zoom; inichanged_ = true; };

private:

	CharSetList charSets_;

	// 全体的な設定
	int        undoLimit_;
	ki::String txtFilter_;
	ki::Path   grepExe_;
	ki::Path   helpExe_;
	ki::String dateFormat_;
//	ki::String timeFormat_;
//	bool datePrior_;

	short      zoom_;
	bool       sharedConfigMode_;
	bool       inichanged_; // keep track of save to ini.

	bool       openSame_;
	bool       countbyunicode_;
	bool       showStatusBar_;
	bool       rememberWindowSize_;
	bool       rememberWindowPlace_;
	bool       useQuickExit_;
	bool       useOldOpenSaveDlg_;
	bool       warnOnModified_;

	// ウインドウサイズ記憶
	bool wndM_; // maximized?
	RECT wndPos_;

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

	void LoadIni() A_COLD;
	void SaveIni() A_COLD;
	void ReadAllDocTypes( const TCHAR *ininame ) A_COLD;
	void LoadLayout( DocType* dt ) A_COLD;
	bool MatchDocType( const unicode* fname, const unicode* pat );

private:

	friend struct ConfigDlg;
	NOCOPY(ConfigManager);
};

//=========================================================================

#endif // AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
