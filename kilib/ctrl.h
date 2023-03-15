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
	void SetParts( int n, int* parts );
	void SetStatusBarVisible(bool b=true);
	void SetParent(HWND parent);

public:

	int width() const;
	bool isStatusBarVisible() const;

private:
	bool Create();
	bool PreTranslateMessage( MSG* ) override A_FINAL;

private:

	int width_;
	bool visible_;
	HWND parent_;
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline int StatusBar::width() const
	{ return width_; }

inline bool StatusBar::isStatusBarVisible() const
	{ return visible_; }

inline void StatusBar::SetParts( int n, int* parts )
	{ SendMsg( SB_SETPARTS, n, reinterpret_cast<LPARAM>(parts) ); }

inline void StatusBar::SetParent(HWND parent)
	{ parent_ = parent; }


#endif // __ccdoc__
//=========================================================================
//@{
//	コンボボックス
//@}
//=========================================================================

class ComboBox A_FINAL: public Window
{
public:
	explicit ComboBox( HWND cb );
	explicit ComboBox( HWND dlg, UINT id );
	void Add( const TCHAR* str );
	void Select( const TCHAR* str );
	int GetCurSel();
	bool GetTextFrom( TCHAR *buf, int len, int i )
	{
		if( i >= 0 )
		{
			int txtlen = SendMsg( CB_GETLBTEXTLEN, i, 0 );
			if( 0 < txtlen && txtlen < len )
			{
				SendMsg( CB_GETLBTEXT, i, (LPARAM)buf );
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



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline ComboBox::ComboBox( HWND cb )
	{ SetHwnd(cb); }

inline ComboBox::ComboBox( HWND dlg, UINT id )
	{ SetHwnd( ::GetDlgItem(dlg,id) ); }

inline void ComboBox::Add( const TCHAR* str )
	{ SendMsg( CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str) ); }

inline int ComboBox::GetCurSel()
	{ return (int) SendMsg( CB_GETCURSEL ); }



//=========================================================================

#endif // __ccdoc__
}      // namespace ki
#endif // _KILIB_CTRL_H_
