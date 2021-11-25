#ifndef _KILIB_CTRL_H_
#define _KILIB_CTRL_H_
#include "window.h"
#ifndef __ccdoc__
namespace ki {
#endif



//=========================================================================
//@{ @pkg ki.Window //@}
//@{
//	�X�e�[�^�X�o�[
//@}
//=========================================================================

class StatusBar : public Window
{
public:

	StatusBar();
	bool Create( HWND parent );
	int  AutoResize( bool maximized );
	void SetText( const TCHAR* str, int part=0 );
	void SetTipText( const TCHAR* tip, int part=0 );
	void SetParts( int n, int* parts );
	void SetStatusBarVisible(bool b=true);

public:

	int width() const;
	bool isStatusBarVisible() const;

private:

	virtual bool PreTranslateMessage( MSG* );

private:

	int width_;
	bool visible_;
};



//-------------------------------------------------------------------------
#ifndef __ccdoc__

inline int StatusBar::width() const
	{ return width_; }

inline bool StatusBar::isStatusBarVisible() const
	{ return visible_; }

inline void StatusBar::SetParts( int n, int* parts )
	{ SendMsg( SB_SETPARTS, n, reinterpret_cast<LPARAM>(parts) ); }

inline void StatusBar::SetText( const TCHAR* str, int part )
	{ SendMsg( SB_SETTEXT, part, reinterpret_cast<LPARAM>(str) ); }

inline void StatusBar::SetStatusBarVisible(bool b)
	{ ::ShowWindow( hwnd(), b?SW_SHOW:SW_HIDE ); visible_=b; }



#endif // __ccdoc__
//=========================================================================
//@{
//	�R���{�{�b�N�X
//@}
//=========================================================================

class ComboBox : public Window
{
public:
	explicit ComboBox( HWND cb );
	explicit ComboBox( HWND dlg, UINT id );
	void Add( const TCHAR* str );
	void Select( const TCHAR* str );
	int GetCurSel();
private:
	virtual bool PreTranslateMessage( MSG* );
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
