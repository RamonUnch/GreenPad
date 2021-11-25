#ifndef _GREENPAD_OPENSAVEDLG_H_
#define _GREENPAD_OPENSAVEDLG_H_
#include "kilib/ktlarray.h"
#include "kilib/ktlaptr.h"
#include "kilib/string.h"
#include "rsrc/resource.h"



//========================================================================
//@{ @pkg Gp.Dlg //@}
//@{
//	���p�\�����R�[�h���X�g
//@}
//========================================================================

class CharSetList
{
public:

	struct CsInfo
	{
		int                 ID;
		const TCHAR*  longName;
		const TCHAR* shortName;
		int               type;
	};

public:

	CharSetList();
	const CsInfo& operator[](size_t i) const { return list_[i]; }
	ulong size() const { return list_.size(); }
	int defaultCs() const;
	ulong defaultCsi() const;
	ulong findCsi( int cs ) const;

private:

	enum { SAVE=1, LOAD=2, BOTH=3 };
	ki::storage<CsInfo> list_;
};



//========================================================================
//@{
//	�u�t�@�C�����J���v�_�C�A���O
//
//	Windows���ʂ̃_�C�A���O�̉��ɁA�����R�[�h�̑I�𗓂�
//	�t�����������̂�\������B
//@}
//========================================================================

class OpenFileDlg
{
public:
	explicit OpenFileDlg( const CharSetList& csl );
	bool DoModal( HWND wnd, const TCHAR* filter, const TCHAR* fnm );

public:
	const TCHAR* filename() const;
	int csi() const;

public:
	static ki::aarr<TCHAR> ConnectWithNull( ki::String lst[], int num );

private:
	const CharSetList& csl_;
	TCHAR filename_[MAX_PATH];
	int   csIndex_;

private:
	static OpenFileDlg* pThis; // �}���`�X���b�h�֎~�I
	static UINT_PTR CALLBACK OfnHook( HWND, UINT, WPARAM, LPARAM );
};



//------------------------------------------------------------------------
#ifndef __ccdoc__

inline OpenFileDlg::OpenFileDlg( const CharSetList& csl )
	: csl_(csl) {}

inline const TCHAR* OpenFileDlg::filename() const
	{ return filename_; }

inline int OpenFileDlg::csi() const
	{ return csIndex_; }



#endif // __ccdoc__
//========================================================================
//@{
//	�u�t�@�C����ۑ��v�_�C�A���O
//
//	Windows���ʂ̃_�C�A���O�̉��ɁA�����R�[�h�̑I�𗓂�
//	���s�R�[�h�̑I�𗓂�t�����������̂�\������B
//@}
//========================================================================

class SaveFileDlg
{
public:
	explicit SaveFileDlg( const CharSetList& csl, int cs, int lb );
	bool DoModal( HWND wnd, const TCHAR* filter, const TCHAR* fnm );

public:
	const TCHAR* filename() const;
	int csi() const;
	int lb() const;

public:
	static ki::aarr<TCHAR> ConnectWithNull( ki::String lst[], int num );

private:
	const CharSetList& csl_;
	TCHAR filename_[MAX_PATH];
	int   csIndex_;
	int   lb_;

private:
	static SaveFileDlg* pThis; // �}���`�X���b�h�֎~�I
	static UINT_PTR CALLBACK OfnHook( HWND, UINT, WPARAM, LPARAM );
};

//------------------------------------------------------------------------
#ifndef __ccdoc__

inline SaveFileDlg::SaveFileDlg( const CharSetList& csl, int cs, int lb )
	: csl_(csl), csIndex_(cs), lb_(lb) {}

inline const TCHAR* SaveFileDlg::filename() const
	{ return filename_; }

inline int SaveFileDlg::csi() const
	{ return csIndex_; }

inline int SaveFileDlg::lb() const
	{ return lb_; }

inline ki::aarr<TCHAR> SaveFileDlg::ConnectWithNull
	( ki::String lst[], int num )
	{ return OpenFileDlg::ConnectWithNull( lst, num ); }



#endif // __ccdoc__
//========================================================================
//@{
//	�u�J�������v�_�C�A���O
//
//	�����R�[�h�I�𗓕\��
//@}
//========================================================================

class ReopenDlg : public ki::DlgImpl
{
public:
	ReopenDlg( const CharSetList& csl, int csi );
	int csi() const;

private:
	void on_init();
	bool on_ok();

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
