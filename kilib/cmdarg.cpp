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
		// 引数を区切る空白をスキップ
		while( *p == TEXT(' ') )
			++p;

		// " だったら、その旨記録してさらに一個進める
		if( *p == TEXT('\"') )
			endc=TEXT('\"'), ++p;
		else
			endc=TEXT(' ');

		// 文字列終端なら終了
		if( *p == TEXT('\0') )
			break;

		// 引数リストへ保存
		if( argNum_ >= MAX_ARGS )
			break;
		
		arg_[ argNum_++ ] = p;


		// 引数の終わりへ…
		while( *p!=endc && *p!=TEXT('\0') )
			++p;

		// 終わりは'\0'にすることによって、引数を区切る
		if( *p != TEXT('\0') )
			*p++ = TEXT('\0');
	}
}
