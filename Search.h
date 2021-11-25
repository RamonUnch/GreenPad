#ifndef AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
#define AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
#include "editwing/editwing.h"
#include "kilib/window.h"
#include "kilib/memory.h"
#include "kilib/ktlaptr.h"
#include "kilib/string.h"



//=========================================================================
//@{ @pkg Gp.Search //@}
//@{
//	�����I�u�W�F�N�g
//@}
//=========================================================================

class Searchable : public ki::Object
{
public:
	//@{
	//	�������s��
	//	@param str �Ώە�����
	//	@param len �Ώە�����̒���
	//	@param stt �����J�nindex�B0�Ȃ�擪����
	//	@param mbg �}�b�`���ʂ̐擪index
	//	@param med �}�b�`���ʂ̏I�[index�̂P���
	//	@return �}�b�`�������ǂ���
	//
	//	�������T�[�`�I�u�W�F�N�g�̏ꍇ�Astt <= *beg �͈̔�
	//	������T�[�`�I�u�W�F�N�g�̏ꍇ�A*beg <= stt �͈̔͂�����
	//@}
	virtual bool Search( const unicode* str, ulong len, ulong stt,
		ulong* mbg, ulong* med ) = 0;
};



//=========================================================================
//@{
//	�����Ǘ��l
//
//	�O�񌟍������Ƃ��̃I�v�V�����⌟����������o���Ă����̂�
//	���̃N���X�̒S���B�����E�u���_�C�A���O�̕\������������
//	��邩������Ȃ��B
//@}
//=========================================================================

class SearchManager : ki::DlgImpl
{
	typedef editwing::DPos DPos;

public:
	//@{ �R���X�g���N�^�B���L�������� //@}
	SearchManager( ki::Window& w, editwing::EwEdit& e );

	//@{ �f�X�g���N�^�B���L�������� //@}
	~SearchManager();

	//@{ �����_�C�A���O�\�� //@}
	void ShowDlg();

	//@{ [��������]�R�}���h //@}
	void FindNext();

	//@{ [�O������]�R�}���h //@}
	void FindPrev();

	//@{ �����������\���H //@}
	bool isReady() const
		{ return searcher_.isValid(); }

	//@{ �ݒ�Save //@}
	void SaveToINI( ki::IniFile& ini );

	//@{ �ݒ�Load //@}
	void LoadFromINI( ki::IniFile& ini );

	//@{ ����̍�^^; //@}
	bool TrapMsg(MSG* msg);

	//@{ ������܂���ł����_�C�A���O //@}
	void NotFound();

private:

	//@{ [�u��]�R�}���h //@}
	void ReplaceImpl();

	//@{ [�S�u��]�R�}���h //@}
	void ReplaceAllImpl();

private:
	
	virtual void on_init();
	virtual void on_destroy();
	virtual bool on_command( UINT cmd, UINT id, HWND ctrl );
	void on_findnext();
	void on_findprev();
	void on_replacenext();
	void on_replaceall();
	void UpdateData();
	void ConstructSearcher( bool down=true );
	void FindNextImpl();
	void FindPrevImpl();
	bool FindNextFromImpl( DPos s, DPos* beg, DPos* end );
	bool FindPrevFromImpl( DPos s, DPos* beg, DPos* end );

private:
	editwing::EwEdit& edit_;
	ki::dptr<Searchable> searcher_;
	ki::Window& mainWnd_;

	bool bIgnoreCase_; // �啶���������𓯈ꎋ�H
	bool bRegExp_;     // ���K�\���H
	bool bDownSearch_; // ��������
	bool bChanged_;    // �O���searcher�\�z������ύX����������true

	ki::String findStr_;
	ki::String replStr_;

private:
	NOCOPY(SearchManager);
};



//=========================================================================

#endif // AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
