#ifndef _KILIB_CTRL_H_
#define _KILIB_CTRL_H_
#include "window.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.Window //@}
//@{
//	ステータスバー
//@}
//=========================================================================

class StatusBar: public Window
{
public:

	StatusBar();
	int  AutoResize( bool maximized );
	void SetText( const TCHAR* str, int part=0 );
	void SetTipText( const TCHAR* tip, int part=0 );
	inline void SetParts( int n, int* parts )
		{ SendMsg( SB_SETPARTS, n, reinterpret_cast<LPARAM>(parts) ); }
	void SetStatusBarVisible(bool b=true);
	inline void SetParent(HWND parent) { parent_ = parent; }

	int GetText( TCHAR* str, int part = 0 );
	int GetTextLen( int part );


public:

	inline int width() const { return width_; }
	inline bool isStatusBarVisible() const { return visible_; }

public:
	class SaveRestoreText
	{
	public:
		SaveRestoreText(ki::StatusBar &stb, int part=0)
		: stb_ ( stb )
		, part_( part )
		{
			buf_[0] = TEXT('\0');
			stb_.GetText( buf_, part_ );
		}
		~SaveRestoreText()
		{
			stb_.SetText( buf_, part_ );
		}
	private:
		TCHAR buf_[256];
		ki::StatusBar& stb_;
		int part_;
	};

private:
	bool Create() A_COLD;
	bool PreTranslateMessage( MSG* ) override A_FINAL;

private:

	int width_;
	bool visible_;
	HWND parent_;
};


//=========================================================================
//@{
//	コンボボックス
//@}
//=========================================================================

class ComboBox A_FINAL: public Window
{
public:
	inline explicit ComboBox( HWND cb ) { SetHwnd(cb); }
	inline explicit ComboBox( HWND dlg, UINT id )
		{ SetHwnd( ::GetDlgItem(dlg,id) ); }
	inline void Add( const TCHAR* str )
		{ SendMsg( CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str) ); }
	void Select( const TCHAR* str );
	inline int GetCurSel() { return (int) SendMsg( CB_GETCURSEL ); }
	bool GetTextFrom( TCHAR *buf, int len, int i )
	{
		if( i >= 0 )
		{
			int txtlen = SendMsg( CB_GETLBTEXTLEN, i, 0 );
			if( 0 < txtlen && txtlen < len )
			{
				SendMsg( CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(buf) );
				return true;
			}
		}
		return false;
	}
	bool GetCurText( TCHAR *buf, int len )
		{ return GetTextFrom( buf, len, GetCurSel() ); }
private:
	bool PreTranslateMessage( MSG* ) override;
};


//=========================================================================

}      // namespace ki
#endif // _KILIB_CTRL_H_
