#ifndef _EDITWING_DOC_H_
#define _EDITWING_DOC_H_
#include "ewCommon.h"
#ifndef __ccdoc__
namespace editwing {
namespace doc {
#endif



class DocImpl;
class DocEvHandler;
class Command;
class Insert;
class Delete;
class Replace;



//=========================================================================
//@{ @pkg editwing.Doc //@}
//@{
//	�����f�[�^
//
//	���̃N���X�͒P�Ȃ�C���^�[�t�F�C�X�ŁA����������
//	class DocImpl �ōs���B�̂ŁA�ڂ����͂�������Q�Ƃ̂��ƁB
//@}
//=========================================================================

class Document : public ki::Object
{
public:

	//@{ �������Ȃ��R���X�g���N�^ //@}
	Document();
	~Document();

	//@{ �t�@�C�����J�� //@}
	void OpenFile( ki::aptr<ki::TextFileR> tf );

	//@{ �t�@�C����ۑ� //@}
	void SaveFile( ki::TextFileW& tf );

	//@{ ���e�j�� //@}
	void ClearAll();

	//@{ ����R�}���h���s //@}
	void Execute( const Command& cmd );

	//@{ �A���h�D //@]
	void Undo();

	//@{ ���h�D //@]
	void Redo();

	//@{ �A���h�D�񐔐��� //@]
	void SetUndoLimit( long lim );

	//@{ �ύX�t���O���N���A //@}
	void ClearModifyFlag();

	//@{ �C�x���g�n���h���o�^ //@}
	void AddHandler( DocEvHandler* eh );

	//@{ �C�x���g�n���h������ //@}
	void DelHandler( DocEvHandler* eh );

	//@{ �L�[���[�h��`�؂�ւ� //@}
	void SetKeyword( const unicode* defbuf, ulong siz=0 );

public:

	//@{ ���������N���X //@}
	DocImpl& impl() { return *impl_; }

	//@{ �s�� //@}
	ulong tln() const;

	//@{ �s�o�b�t�@ //@}
	const unicode* tl( ulong i ) const;

	//@{ �s������ //@}
	ulong len( ulong i ) const;

	//@{ �w��͈͂̃e�L�X�g�̒��� //@}
	ulong getRangeLength( const DPos& stt, const DPos& end ) const;

	//@{ �w��͈͂̃e�L�X�g //@}
	void getText( unicode* buf, const DPos& stt, const DPos& end ) const;

	//@{ �A���h�D�\�H //@}
	bool isUndoAble() const;

	//@{ ���h�D�\�H //@}
	bool isRedoAble() const;

	//@{ �ύX�ς݁H //@}
	bool isModified() const;

	//@{ �r�W�[�t���O�i�}�N���R�}���h���s���̂ݐ����j //@}
	void setBusyFlag( bool b=true ) { busy_ = b; }
	bool isBusy() const { return busy_; }

private:

	// ����
	ki::dptr<DocImpl> impl_;
	bool busy_;

private:

	NOCOPY(Document);
};



//=========================================================================
//@{
//	�C�x���g�n���h���C���^�[�t�F�C�X
//
//	�h�L�������g���甭������C�x���g�i�}��/�폜�ȂǂȂǁc�j��
//	�󂯎�肽���ꍇ�́A���̃C���^�[�t�F�C�X���p�����A�K�X�n���h����
//	�������ƁBView�̍ĕ`�揈���Ȃǂ������ʂ��Ď��s����Ă���B
//@}
//=========================================================================

class DocEvHandler
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

class Command : public ki::Object
{
protected:
	friend class UnReDoChain;
	friend class MacroCommand;
	virtual Command* operator()( Document& doc ) const = 0;
};



//=========================================================================
//@{
//	�}���R�}���h
//@}
//=========================================================================

class Insert : public Command
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

	Command* operator()( Document& doc ) const;
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

class Delete : public Command
{
public:

	//@{
	//	@param s �J�n�ʒu
	//	@param e �I�[�ʒu
	//@}
	Delete( const DPos& s, const DPos& e );

private:

	Command* operator()( Document& doc ) const;
	DPos stt_;
	DPos end_;
};




//=========================================================================
//@{
//	�u���R�}���h
//@}
//=========================================================================

class Replace : public Command
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

	Command* operator()( Document& doc ) const;
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

class MacroCommand : public Command
{
public:
	//@{ �R�}���h�̒ǉ� //@}
	void Add( Command* cmd ) { arr_.Add(cmd); }

	//@{ �R�}���h�� //@}
	ulong size() const { return arr_.size(); }

	//@ �f�X�g���N�^ //@}
	~MacroCommand()
	{
		for( ulong i=0,e=arr_.size(); i<e; ++i )
			delete arr_[i];
	}

private:
	Command* operator()( Document& doc ) const;
	ki::storage<Command*> arr_;
};



//=========================================================================

}}     // namespace editwing::document
#endif // _EDITWING_DOC_H_
