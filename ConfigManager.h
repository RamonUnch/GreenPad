#ifndef AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#define AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
#include "editwing/editwing.h"
#include "OpenSaveDlg.h"



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

class ConfigManager : public ki::Object
{
public:

	ConfigManager();
	~ConfigManager();

	//@{ �w�肵�����O�̃t�@�C���p�̕����^�C�v�����[�h //@}
	int SetDocType( const ki::Path& fname );

	//@{ �w�肵���ԍ��̕����^�C�v�����[�h //@}
	void SetDocTypeByMenu( int pos, HMENU m );

	//@{ �w�肵�����O�̕����^�C�v�����[�h //@}
	void SetDocTypeByName( const ki::String& nam );

	//@{ ���j���[���ڍ쐬 //@}
	void SetDocTypeMenu( HMENU m, UINT idstart );

	//@{ ���j���[���ڂ̃`�F�b�N�C�� //@}
	void CheckMenu( HMENU m, int pos );

	//@{ �ݒ�_�C�A���O�\�� //@}
	bool DoDialog( const ki::Window& parent );

	//@{ ����ini�t�@�C������I�u�W�F�N�g���擾 //@}
	ki::IniFile& getImpl();

public:

	//@{ Undo�񐔐����l //@}
	int undoLimit() const;

	//@{ �������̃J�E���g���@ //@}
	bool countByUnicode() const;

	//@{ �J��/�ۑ��_�C�A���O�ɏo���t�B���^�̐ݒ� //@}
	const ki::String& txtFileFilter() const;

	//@{ �������w�莞�̐܂�Ԃ������� //@}
	int wrapWidth() const;

	//@{ �܂�Ԃ����@ //@}
	int wrapType() const;

	//@{ �s�ԍ��\������H //@}
	bool showLN() const;

	//@{ �\���F�E�t�H���g�Ȃ� //@}
	const editwing::VConfig& vConfig() const;

	//@{ �L�[���[�h�t�@�C����(�t���p�X) //@}
	ki::Path kwdFile() const;

	//@{ Grep�p�O�����s�t�@�C���� //@}
	const ki::Path& grepExe() const;

	//@{ �����E�C���h�E�ŊJ�����[�h //@}
	bool openSame() const;

	//@{ �X�e�[�^�X�o�[�\�� //@}
	bool showStatusBar() const;
	void ShowStatusBarSwitch();

	//@{ ���t //@}
	const ki::String& dateFormat() const;

public:
	//@{ �V�K�t�@�C���̕����R�[�hindex //@}
	int GetNewfileCsi() const;

	//@{ �V�K�t�@�C���̉��s�R�[�h //@}
	ki::lbcode GetNewfileLB() const;

public:
	//@{ [�ŋߎg�����t�@�C��]�֒ǉ� //@}
	void AddMRU( const ki::Path& fname );

	//@{ [�ŋߎg�����t�@�C��]���j���[�̍\�z //@}
	void SetUpMRUMenu( HMENU m, UINT id );

	//@{ [�ŋߎg�����t�@�C��]�擾 //@}
	ki::Path GetMRU( int no ) const;

	//@{ �Ή������Z�b�g���X�g�擾 //@}
	CharSetList& GetCharSetList();

public:
	//@{ �E�C���h�E�ʒu�E�T�C�Y�������� //@}
	int GetWndX() const;
	int GetWndY() const;
	int GetWndW() const;
	int GetWndH() const;
	bool GetWndM() const;
	void RememberWnd( ki::Window* wnd );

private:

	ki::IniFile ini_;
	bool        sharedConfigMode_;
	CharSetList charSets_;

	// �S�̓I�Ȑݒ�
	int        undoLimit_;
	ki::String txtFilter_;
	ki::Path   grepExe_;
	bool       openSame_;
	bool       countbyunicode_;
	bool       showStatusBar_;
	bool       rememberWindowSize_;
	bool       rememberWindowPlace_;

	ki::String dateFormat_;
//	ki::String timeFormat_;
//	bool datePrior_;

	// �E�C���h�E�T�C�Y�L��
	bool wndM_; // maximized?
	int  wndX_, wndY_, wndW_, wndH_;

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
		int               wrapType;
		int               wrapWidth;
		bool              showLN;
		char              fontCS;
		int               fontQual;
	};
	typedef ki::olist<DocType> DtList;

	DtList           dtList_;
	DtList::iterator curDt_;

	// �ŋߎg�����t�@�C���̃��X�g
	int mrus_;
	ki::Path mru_[20];

	// �V�K�t�@�C���֌W
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

inline int ConfigManager::wrapWidth() const
	{ return curDt_->wrapWidth; }

inline int ConfigManager::wrapType() const
	{ return curDt_->wrapType>0 ? wrapWidth() : curDt_->wrapType; }

inline bool ConfigManager::showLN() const
	{ return curDt_->showLN; }

inline const editwing::VConfig& ConfigManager::vConfig() const
	{ return curDt_->vc; }

inline ki::Path ConfigManager::kwdFile() const
	{ return ki::Path(ki::Path::Exe)+TEXT("type\\")+curDt_->kwdfile; }

inline const ki::Path& ConfigManager::grepExe() const
	{ return grepExe_; }

inline bool ConfigManager::openSame() const
	{ return openSame_; }

inline bool ConfigManager::showStatusBar() const
	{ return showStatusBar_; }

inline void ConfigManager::ShowStatusBarSwitch()
	{ showStatusBar_ = !showStatusBar_; SaveIni(); }

inline bool ConfigManager::countByUnicode() const
	{ return countbyunicode_; }

inline ki::IniFile& ConfigManager::getImpl()
	{ return ini_; }

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

//=========================================================================

#endif // __ccdoc__
#endif // AFX_ONFIGMANAGER_H__9243DE9D_0F70_40F8_8F90_55436B952B37__INCLUDED_
