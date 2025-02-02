#include "stdafx.h"
#include "cmdarg.h"
#include "kstring.h"
using namespace ki;



//=========================================================================

Argv::Argv( const TCHAR* cmd )
: argNum_ ( 0 )
{
	TCHAR *p, endc;

	buf_ = p = (TCHAR*)malloc( sizeof(TCHAR) * (my_lstrlen(cmd)+1) );
	if( !p ) return;
	my_lstrcpy( p, cmd );

	while( *p != TEXT('\0') )
	{
		// ��������؂�󔒂��X�L�b�v
		while( *p == TEXT(' ') )
			++p;

		// " ��������A���̎|�L�^���Ă���Ɉ�i�߂�
		if( *p == TEXT('\"') )
			endc=TEXT('\"'), ++p;
		else
			endc=TEXT(' ');

		// ������I�[�Ȃ�I��
		if( *p == TEXT('\0') )
			break;

		// �������X�g�֕ۑ�
		if( argNum_ >= MAX_ARGS )
			break;
		
		arg_[ argNum_++ ] = p;


		// �����̏I���ցc
		while( *p!=endc && *p!=TEXT('\0') )
			++p;

		// �I����'\0'�ɂ��邱�Ƃɂ���āA��������؂�
		if( *p != TEXT('\0') )
			*p++ = TEXT('\0');
	}
}
