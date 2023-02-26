#ifndef _GREENPAD_MAIN_H_
#define _GREENPAD_MAIN_H_
#include "kilib/kilib.h"
#include "editwing/editwing.h"
#include "OpenSaveDlg.h"
#include "ConfigManager.h"
#include "Search.h"



//=========================================================================
//@{ @pkg Gp.Main //@}
//@{
//	ステータスバー
//@}
//=========================================================================

class GpStBar : public ki::StatusBar
{
public:
	GpStBar();
	int AutoResize( bool maximized );
	void SetCsText( const TCHAR* str );
	void SetLbText( int lb );
private:
	const TCHAR *str_;
	int lb_;
};



//=========================================================================
//@{
//	メインウインドウ
//@}
//=========================================================================

class GreenPadWnd
	: public ki::WndImpl
	, public editwing::doc::DocEvHandler
	, public editwing::view::CurEvHandler
{
public:

	GreenPadWnd();
	bool StartUp( const ki::Path& fn, int cs, int ln );
	void ShowUp2();
	
private:

	void UpdateWindowName();
	void ReloadConfig( bool noSetDocType=false );

	bool ShowOpenDlg( ki::Path* fn, int* cs );
	bool Open( const ki::Path& fn, int cs, bool always=true );
	bool OpenByMyself( const ki::Path& fn, int cs, bool needReConf=true, bool always=false );
	static BOOL CALLBACK PostMsgToFriendsProc(HWND hwnd, LPARAM lPmsg);
	BOOL PostMsgToAllFriends(UINT msg);
	BOOL myPageSetupDlg(LPPAGESETUPDLG lppsd);

	bool AskToSave();
	bool Save_showDlgIfNeeded();
	bool ShowSaveDlg();
	bool Save();

	void JumpToLine( ulong ln );
	void SetupSubMenu();
	void SetupMRUMenu();

private:

	bool isUntitled() const { return filename_.len()==0; }
	int resolveCSI(int csi) const ;

private:

	ConfigManager    cfg_;
	SearchManager    search_;
	CharSetList&     charSets_;

	editwing::EwEdit edit_;
	GpStBar          stb_;
	HACCEL           accel_;

	ki::Path         filename_;
	int              csi_;
	int              lb_;
	int              wrap_;
//	int              clickHT_;
//	short            clickX_;
//	short            clickY_;

	static ClsName   className_;

private:

	void    on_create( CREATESTRUCT* cs );
	LRESULT on_message( UINT msg, WPARAM wp, LPARAM lp );
	bool    on_command( UINT id, HWND ctrl );
	void    on_newfile();
	void    on_openfile();
	void    on_reopenfile();
	void    on_openelevated(const ki::Path& fn);
	void    on_refreshfile();
	void    on_savefile();
	void    on_savefileas();
	void	SetFontSizeforDC(LOGFONT *font, HDC hDC, int fsiz, int fx);
	void    on_print();
	void    on_pagesetup();
	void    on_exit();
	void    on_initmenu( HMENU menu, bool editmenu_only );
	void    on_drop( HDROP hd );
	void    on_dirtyflag_change( bool );
	void    on_move( const editwing::DPos& c, const editwing::DPos& s );
	void    on_jump();
	void    on_grep();
	void    on_config();
	void    on_datetime();
	void    on_doctype( int no );
	void    on_nextwnd();
	void    on_prevwnd();
	void    on_mru( int no );
	void    on_statusBar();
	void    on_reconv();
	void    on_toggleime();
	bool    PreTranslateMessage( MSG* msg );
};



//=========================================================================

#endif // _GREENPAD_MAIN_H_
