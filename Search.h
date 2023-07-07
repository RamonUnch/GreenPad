#ifndef AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
#define AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
#include "editwing/editwing.h"
#include "kilib/window.h"
#include "kilib/memory.h"
#include "kilib/kstring.h"
#include "kilib/registry.h"



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

	virtual ~Searchable() {};
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

class SearchManager A_FINAL: public ki::DlgImpl
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
		{ return searcher_ != NULL; }

	//@{ �ݒ�Save //@}
	void SaveToINI();

	//@{ �ݒ�Load //@}
	void LoadFromINI();

	//@{ ����̍�^^; //@}
	bool TrapMsg(MSG* msg);

	//@{ ������܂���ł����_�C�A���O //@}
	void NotFound(bool GoingDown=false);

private:

	//@{ [�u��]�R�}���h //@}
	void ReplaceImpl();

	//@{ [�S�u��]�R�}���h //@}
	void ReplaceAllImpl();

private:

	void on_init() override;
	void on_destroy() override;
	bool on_command( UINT cmd, UINT id, HWND ctrl ) override;
	bool on_cancel() override;

	void on_findnext();
	void on_findprev();
	void on_replacenext();
	void on_replaceall();
	void UpdateData();
	void ConstructSearcher( bool down=true );
	void FindNextImpl( bool redo=false );
	void FindPrevImpl();
	bool FindNextFromImpl( DPos s, DPos* beg, DPos* end );
	bool FindPrevFromImpl( DPos s, DPos* beg, DPos* end );

private:
	editwing::EwEdit& edit_;
	Searchable *searcher_;
	ki::Window& mainWnd_;

	bool bIgnoreCase_; // �啶���������𓯈ꎋ�H
	bool bRegExp_;     // ���K�\���H
	bool bDownSearch_; // ��������
	bool bChanged_;    // �O���searcher�\�z������ύX����������true
	bool inichanged_;  // Set to true when the ini must be saved

	ki::String findStr_;
	ki::String replStr_;

private:
	NOCOPY(SearchManager);
};



//=========================================================================

#endif // AFX_SEARCH_H__201E0D70_9C20_420A_8600_966D2BA23010__INCLUDED_
