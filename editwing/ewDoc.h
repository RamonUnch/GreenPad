#ifndef _EDITWING_DOC_H_
#define _EDITWING_DOC_H_
#include "ewCommon.h"
#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



class Document;
class DocEvHandler;
class Command;
class Insert;
class Delete;
class Replace;


//=========================================================================
//@{
//	�C�x���g�n���h���C���^�[�t�F�C�X
//
//	�h�L�������g���甭������C�x���g�i�}��/�폜�ȂǂȂǁc�j��
//	�󂯎�肽���ꍇ�́A���̃C���^�[�t�F�C�X���p�����A�K�X�n���h����
//	�������ƁBView�̍ĕ`�揈���Ȃǂ������ʂ��Ď��s����Ă���B
//@}
//=========================================================================

class A_NOVTABLE DocEvHandler
{
public:
	//@{
	//	�e�L�X�g���e���ύX���ꂽ�Ƃ��ɔ���
	//	@param s        �ύX�͈͂̐擪
	//	@param e        �ύX�͈͂̏I�[(�O)
	//	@param e2       �ύX�͈͂̏I�[(��)
	//	@param reparsed e2�����̃R�����g�A�E�g��Ԃ��ω����Ă�����true
	//	@param nmlcmd   �}��/�폜/�u���Ȃ�true�A�t�@�C���J��/�S�u���Ȃ�false
	//@}
	virtual void on_text_update( const DPos& s,
		const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd ) {}

	//@{
	//	�L�[���[�h���ύX���ꂽ�Ƃ��ɔ���
	//@}
	virtual void on_keyword_change() {}

	//@{
	//	�_�[�e�B�t���O���ύX���ꂽ�Ƃ��ɔ���
	//@}
	virtual void on_dirtyflag_change( bool dirty ) {}
};



//=========================================================================
//@{
//	����R�}���h�C���^�[�t�F�C�X
//
//	�h�L�������g�́ACommand ����h�������N���X�̃C���X�^���X��
//	operator() ���Ăяo�����ƂŁA�F�X�ȑ�������s����B�Ƃ肠����
//	��̓I�ɂ� Insert/Delete/Replace �̂R�����B���ƂŃ}�N���R�}���h�p
//	�N���X�������肾���ǁA�Ƃ肠�����͕ۗ��B
//@}
//=========================================================================

class A_NOVTABLE Command : public ki::Object
{
protected:
	friend class UnReDoChain;
	friend class MacroCommand;
	virtual Command* operator()( Document& doc ) const = 0;
public:
	virtual ~Command() {};
};



//=========================================================================
//@{
//	�}���R�}���h
//@}
//=========================================================================

class Insert A_FINAL: public Command
{
public:

	//@{
	//	@param s �}���ʒu
	//	@param str �}��������
	//	@param len ������̒���
	//	@param del �R�}���h�I������delete [] str���Ă悢���H
	//@}
	Insert( const DPos& s, const unicode* str, ulong len, bool del=false );
	~Insert();

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	const unicode* buf_;
	ulong          len_;
	bool           del_;
};



//=========================================================================
//@{
//	�폜�R�}���h
//@}
//=========================================================================

class Delete A_FINAL: public Command
{
public:

	//@{
	//	@param s �J�n�ʒu
	//	@param e �I�[�ʒu
	//@}
	Delete( const DPos& s, const DPos& e );

private:

	Command* operator()( Document& doc ) const override;
	DPos stt_;
	DPos end_;
};




//=========================================================================
//@{
//	�u���R�}���h
//@}
//=========================================================================

class Replace A_FINAL: public Command
{
public:

	//@{
	//	@param s �J�n�ʒu
	//	@param e �I�[�ʒu
	//	@param str �}��������
	//	@param len ������̒���
	//	@param del �R�}���h�I������delete [] str���Ă悢���H
	//@}
	Replace( const DPos& s, const DPos& e,
		const unicode* str, ulong len, bool del=false );
	~Replace();

private:

	Command* operator()( Document& doc ) const override;
	DPos           stt_;
	DPos           end_;
	const unicode* buf_;
	ulong          len_;
	bool           del_;
};



//=========================================================================
//@{
//	�}�N���R�}���h
//
//	�����̃R�}���h����̃R�}���h�Ƃ��ĘA�����s����B
//	�������AInsert/Delete/Replace�����s�����тɓ��R
//	������̈ʒu�͕ω�����̂����A����Ɋւ���ϊ�������
//	�s��Ȃ��B���Ȃ킿�AInsert->Delete->Insert �݂�����
//	�A�������������Ƃ��́A�s���╶�����̕ω����l�����Ȃ���
//	�l���߂Ă������Ƃ��K�v�ɂȂ�B�̂ŁA����܂�g���Ȃ�(^^;
//@}
//=========================================================================

class MacroCommand A_FINAL: public Command
{
public:
	//@{ �R�}���h�̒ǉ� //@}
	void Add( Command* cmd ) { if( cmd ) arr_.Add(cmd); } // do not save NULLs

	//@{ �R�}���h�� //@}
	ulong size() const { return arr_.size(); }

	//@ �f�X�g���N�^ //@}
	~MacroCommand()
	{
		for( ulong i=0,e=arr_.size(); i<e; ++i )
			delete arr_[i];
	}

private:
	Command* operator()( Document& doc ) const override;
	ki::storage<Command*> arr_;
};



//=========================================================================

}}     // namespace editwing::document
#endif // _EDITWING_DOC_H_
