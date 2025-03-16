#ifndef AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#define AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#include "editwing/editwing.h"
#include "OpenSaveDlg.h"
#include "kilib/registry.h"

void SetFontSize(LOGFONT *font, HDC hDC, int fsiz, int fx);

// �A�v���P�[�V�������b�Z�[�W
#define GPM_MRUCHANGED WM_APP+0

//=========================================================================
//@{ @pkg Gp.Main //@}
//@{
//	�ݒ�̈ꌳ�Ǘ�
//
//	SetDocType�Ő؂�ւ���ƁA�����^�C�v�ˑ��̍��ڂ������
//	�K�؂ɐ؂�ւ�����F�X���܂��B
//@}
//=========================================================================

class ConfigManager
{
public:

	ConfigManager() A_COLD;
	~ConfigManager() A_COLD;

	//@{ �w�肵�����O�̃t�@�C���p�̕����^�C�v�����[�h //@}
	int SetDocType( const ki::Path& fname )  A_COLD;

	//@{ �w�肵���ԍ��̕����^�C�v�����[�h //@}
	void SetDocTypeByMenu( int pos, HMENU m )  A_COLD;

	//@{ �w�肵�����O�̕����^�C�v�����[�h //@}
	bool SetDocTypeByName( const ki::String& nam )  A_COLD;

	//@{ ���j���[���ڍ쐬 //@}
	void SetDocTypeMenu( HMENU m, UINT idstart )  A_COLD;

	//@{ ���j���[���ڂ̃`�F�b�N�C�� //@}
	void CheckMenu( HMENU m, int pos )  A_COLD;

	//@{ �ݒ�_�C�A���O�\�� //@}
	bool DoDialog( const ki::Window& parent )  A_COLD;

public:

	//@{ Undo�񐔐����l //@}
	inline int undoLimit() const { return undoLimit_; }

	//@{ �������̃J�E���g���@ //@}
	inline bool countByUnicode() const { return countbyunicode_; }

	//@{ �J��/�ۑ��_�C�A���O�ɏo���t�B���^�̐ݒ� //@}
	inline const ki::String& txtFileFilter() const { return txtFilter_; }

	//@{ �������w�莞�̐܂�Ԃ������� //@}
	inline short wrapWidth() const { return curDt_->wrapWidth; }

	//@{ Get smart warp flag //@}
	inline bool wrapSmart() const { return curDt_->wrapSmart; }

	//@{ �܂�Ԃ����@ //@}
	inline short wrapType() const { return curDt_->wrapType>0 ? wrapWidth() : curDt_->wrapType; }

	//@{ �s�ԍ��\������H //@}
	inline bool showLN() const { return curDt_->showLN; }

	//@{ �\���F�E�t�H���g�Ȃ� //@}
	inline const editwing::VConfig& vConfig() const { return curDt_->vc; }

	//@{ �L�[���[�h�t�@�C����(�t���p�X) //@}
	inline ki::Path kwdFile() const
		{
		ki::Path p(ki::Path::Exe);
		p += TEXT("type\\");
		p += curDt_->kwdfile;
		return p;
		}

	//@{ Grep�p�O�����s�t�@�C���� //@}
	inline const ki::Path& grepExe() const { return grepExe_; }

	//@{ Help�p�O�����s�t�@�C���� //@}
	inline const ki::Path& helpExe() const { return helpExe_; }

	//@{ �����E�C���h�E�ŊJ�����[�h //@}
	inline bool openSame() const { return openSame_; }

	//@{ �X�e�[�^�X�o�[�\�� //@}
	inline bool showStatusBar() const { return showStatusBar_; }
	inline void ShowStatusBarSwitch() { showStatusBar_ = !showStatusBar_; inichanged_=1; SaveIni(); }

	//@{ ���t //@}
	inline const ki::String& dateFormat() const { return dateFormat_; }

public:
	//@{ �V�K�t�@�C���̕����R�[�hindex //@}
	inline int GetNewfileCsi() const { return charSets_.findCsi( newfileCharset_ ); }

	//@{ �V�K�t�@�C���̉��s�R�[�h //@}
	inline ki::lbcode GetNewfileLB() const { return newfileLB_; }

public:
	//@{ [�ŋߎg�����t�@�C��]�֒ǉ� //@}
	bool AddMRU( const ki::Path& fname )  A_COLD;

	//@{ [�ŋߎg�����t�@�C��]���j���[�̍\�z //@}
	int SetUpMRUMenu( HMENU m, UINT id )  A_COLD;

	//@{ [�ŋߎg�����t�@�C��]�擾 //@}
	ki::Path GetMRU( int no ) const A_COLD;

	//@{ �Ή������Z�b�g���X�g�擾 //@}
	inline CharSetList& GetCharSetList() { return charSets_; }

public:
	//@{ �E�C���h�E�ʒu�E�T�C�Y�������� //@}
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

	// �S�̓I�Ȑݒ�
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

	// �E�C���h�E�T�C�Y�L��
	bool wndM_; // maximized?
	RECT wndPos_;

	// �����^�C�v�̃��X�g
	struct DocType
	{
		// ��`�t�@�C�����Ȃ�
		ki::String        name;
		ki::String        pattern;
		ki::String        kwdfile;
		ki::String        layfile;

		// �ݒ荀��
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

	// �ŋߎg�����t�@�C���̃��X�g
	int mrus_;
	ki::Path mru_[20];

	// �V�K�t�@�C���֌W
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
