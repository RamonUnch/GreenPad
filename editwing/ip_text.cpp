
#include "../kilib/stdafx.h"
#include "ip_doc.h"
using namespace ki;
using namespace editwing;
using namespace editwing::doc;



//=========================================================================
//---- ip_text.cpp   文字列操作・他
//
//		文字列を挿入したり削除したり…という辺りの処理が
//		このファイルにまとめてある。外向けのインターフェイスの
//		実装もついでにここで。
//
//---- ip_parse.cpp  キーワード解析
//---- ip_wrap.cpp   折り返し
//---- ip_scroll.cpp スクロール
//---- ip_draw.cpp   描画・他
//---- ip_cursor.cpp カーソルコントロール
//=========================================================================


//-------------------------------------------------------------------------
// イベントハンドラ処理
//-------------------------------------------------------------------------

void Document::AddHandler( DocEvHandler* eh )
{
	// ハンドラ追加
	pEvHan_.Add( eh );
}

void Document::DelHandler( const DocEvHandler* eh )
{
	// 後ろから見て行って…
	const int last = pEvHan_.size() - 1;

	// …見つけたら削除
	for( int i=last; i>=0; --i )
		if( pEvHan_[i] == eh )
		{
			pEvHan_[i] = pEvHan_[last];
			pEvHan_.ForceSize( last );
			break;
		}
}

//void Document::acc_Fire_TEXTUPDATE_begin()
//{
//	acc_textupdate_mode = true;
//	acc_s_ = DPos(-1, -1);
//	acc_e2_ = DPos(0,0);
//	acc_reparsed_ = acc_nmlcmd_ = false;
//}
//void Document::acc_Fire_TEXTUPDATE_end()
//{
//	acc_textupdate_mode = false;
//	Fire_TEXTUPDATE( acc_s_, acc_e2_, acc_e2_, acc_reparsed_, acc_nmlcmd_ );
//}

void Document::Fire_TEXTUPDATE
	( const DPos& s, const DPos& e, const DPos& e2, bool reparsed, bool nmlcmd )
{
//	if( acc_textupdate_mode )
//	{
//		acc_s_  = Min(acc_s_, s);
//		acc_e2_ = Max(acc_e2_, e2);
//		acc_reparsed_ = acc_reparsed_ || reparsed;
//		acc_nmlcmd_   = acc_nmlcmd_ || nmlcmd;
//		//LOGGERF( TEXT("s=%u,%lu, e=%lu,%lu, e2=%lu,%lu, rp=%d, nm=%d")
//		//       , s.tl, s.ad, e.tl, e.ad, e2.tl, e2.ad, (int)reparsed, (int)nmlcmd );
//		LOGGERF( TEXT("s=%u,%lu, e2=%lu,%lu, rp=%d, nm=%d")
//		       , acc_s_.tl, acc_s_.ad,  acc_e2_.tl, acc_e2_.ad, (int)reparsed, (int)nmlcmd );
//	}
//	else
	{
		AutoLock lk(this);

		// 全部にイベント通知
		for( ulong i=0, ie=pEvHan_.size(); i<ie; ++i )
			pEvHan_[i]->on_text_update( s, e, e2, reparsed, nmlcmd );
	}
}

void Document::Fire_KEYWORDCHANGE()
{
	AutoLock lk(this);

	// 全部にイベント通知
	for( ulong i=0, ie=pEvHan_.size(); i<ie; ++i )
		pEvHan_[i]->on_keyword_change();
}

void Document::Fire_MODIFYFLAGCHANGE()
{
	AutoLock lk(this);

	// 全部にイベント通知
	bool b = urdo_.isModified();
	for( ulong i=0, ie=pEvHan_.size(); i<ie; ++i )
		pEvHan_[i]->on_dirtyflag_change( b );
}



//-------------------------------------------------------------------------
// UnDo,ReDo 処理
//-------------------------------------------------------------------------

UnReDoChain::Node::Node( Command* c, Node* p, Node* n )
	: next_( n )
	, prev_( p )
	, cmd_ ( c )
{
}

UnReDoChain::Node::Node()
{
	next_ = prev_ = this;
	cmd_  = NULL;
}

UnReDoChain::Node::~Node()
{
	delete cmd_;
}

void UnReDoChain::Node::ResetCommand( Command* cmd )
{
	delete cmd_;
	cmd_ = cmd;
}

UnReDoChain::UnReDoChain()
	: savedPos_( &headTail_ )
	, lastOp_  ( &headTail_ )
	, num_     ( 0 )
	, limit_   ( static_cast<ulong>(-1) )
{
}

UnReDoChain::~UnReDoChain()
{
	Clear();
}

ulong UnReDoChain::Node::ChainDelete(Node*& savedPos_ref)
{
	if( cmd_ == NULL )
		return 0;
	if( savedPos_ref == this )
		savedPos_ref = NULL;
	ulong ret = 1 + next_->ChainDelete(savedPos_ref);
	delete this; // Delete this node.
	return ret;
}

void UnReDoChain::Clear()
{
	headTail_.next_->ChainDelete(savedPos_);
	headTail_.next_ = headTail_.prev_ = lastOp_  = savedPos_ = &headTail_;
	num_ = 0;
}

void UnReDoChain::SetLimit( long lim )
{
	limit_ = Max( (ulong)1, ulong(lim) );
}

inline void UnReDoChain::Undo( Document& doc )
{
	lastOp_->ResetCommand( (*lastOp_->cmd_)(doc) );
	lastOp_ = lastOp_->prev_;
}

inline void UnReDoChain::Redo( Document& doc )
{
	lastOp_ = lastOp_->next_;
	lastOp_->ResetCommand( (*lastOp_->cmd_)(doc) );
}

void UnReDoChain::NewlyExec( const Command& cmd, Document& doc )
{
	Command* nCmd = cmd(doc);
	if( nCmd != NULL )
	{
		Node *nn = new Node(nCmd,lastOp_,&headTail_);
		if( !nn )
		{	// Unable to allocate the Node!
			// delete now the command that can't be Saved...
			delete nCmd;
			return;
		}
		num_   -= (lastOp_->next_->ChainDelete(savedPos_) - 1);
		lastOp_ = lastOp_->next_ = nn;

		while( limit_ < num_ )
		{
			// 回数制限を越えたので、古い物を削除
			// Deleted the old one because it exceeded the count limit.
			Node* old = headTail_.next_;
			headTail_.next_   = old->next_;
			old->next_->prev_ = &headTail_;
			if( old != &headTail_ )
				delete old;
			if( savedPos_ == &headTail_ )
				savedPos_ = NULL;
			else if( savedPos_ == old )
				savedPos_ = &headTail_;
			--num_;
		}
	}
}



bool Document::isUndoAble() const
	{ return urdo_.isUndoAble(); }

bool Document::isRedoAble() const
	{ return urdo_.isRedoAble(); }

bool Document::isModified() const
	{ return urdo_.isModified(); }

void Document::SetUndoLimit( long lim )
	{ urdo_.SetLimit( lim ); }

void Document::ClearModifyFlag()
{
	bool b = urdo_.isModified();
	urdo_.SavedHere();
	if( b != urdo_.isModified() )
		Fire_MODIFYFLAGCHANGE();
}

void Document::Undo()
{
	if( urdo_.isUndoAble() )
	{
		bool b = urdo_.isModified();
		urdo_.Undo(*this);
		if( b != urdo_.isModified() )
			Fire_MODIFYFLAGCHANGE();
	}
}

void Document::Redo()
{
	if( urdo_.isRedoAble() )
	{
		bool b = urdo_.isModified();
		urdo_.Redo(*this);
		if( b != urdo_.isModified() )
			Fire_MODIFYFLAGCHANGE();
	}
}

void Document::Execute( const Command& cmd )
{
	bool b = urdo_.isModified();
	urdo_.NewlyExec( cmd, *this );
	if( b != urdo_.isModified() )
		Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// カーソル移動ヘルパー, Cursor movement helper
//-------------------------------------------------------------------------

DPos Document::leftOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == 0 )
	{
		// 行の先頭だが、ファイルの先頭ではない場合
		// 一つ前の行の行末へ
		// If beginning of a line, but not the beginning of a file
		// Go to the end of the previous line.
		if( dp.tl > 0 )
			return DPos( dp.tl-1, len(dp.tl-1) );
		return dp;
	}
	else if( !wide )
	{
		// 行の途中で、普通に１文字戻る場合
		// If you want to go back one character "normally", in the middle of a line
		const unicode* l = tl(dp.tl);
		if( dp.ad>=2 && isLowSurrogate(l[dp.ad-1]) && isHighSurrogate(l[dp.ad-2]) )
			return DPos( dp.tl, dp.ad-2 );
		return DPos( dp.tl, dp.ad-1 );
	}
	else
	{
		// 行の途中で、１単語分戻る場合, To go back one word in the middle of a line
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad;
		while( --s && (f[s]>>5)==0 );
		return DPos( dp.tl, s );
	}
}

DPos Document::rightOf( const DPos& dp, bool wide ) const
{
	if( dp.ad == len(dp.tl) )
	{
		// 行末だが、ファイルの終わりではない場合
		// 一つ後の行の先頭へ
		// At the end of a line, but not at the end of a file
		// Go to the beginning of the next line.
		if( dp.tl < tln()-1 )
			return DPos( dp.tl+1, 0 );
		return dp;
	}
	else if( !wide )
	{
		// 行の途中で、普通に１文字進む場合
		// If you advance one character normally in the middle of a line
		const unicode* l = tl(dp.tl);
		// 番兵 0x007f が l の末尾にいるので長さチェックは不要
		// No need to check the length of l with the #0x007f guard at the end of l.
		if( isHighSurrogate(l[dp.ad]) && isLowSurrogate(l[dp.ad+1]) )
			return DPos( dp.tl, dp.ad+2 );
		return DPos( dp.tl, dp.ad+1 );
	}
	else
	{
		// 行の途中で、普通に１単語進む場合
		// If you advance one word normally in the middle of a line
		const uchar* f = pl(dp.tl);
		const ulong  e = len(dp.tl);
			  ulong  s = dp.ad;
		const ulong  t = (f[s]>>5);
		s += t;
		if( s >= e )
			s = e;
		else if( t==7 || t==0 )
			while( s<e && (f[s]>>5)==0 )
				++s;
		return DPos( dp.tl, s );
	}
}

DPos Document::wordStartOf( const DPos& dp ) const
{
	if( dp.ad == 0 )
	{
		// 行の先頭, Beginning of line
		return dp;
	}
	else
	{
		// 行の途中, Midline
		const uchar* f = pl(dp.tl);
			  ulong  s = dp.ad+1;
		while( --s && (f[s]>>5)==0 );

		return DPos( dp.tl, s );
	}
}

DPos Document::findMatchingBrace( const DPos &dp ) const
{
	// Currently selected character.
	DPos np=dp;
	bool backward, backside=0;
	unicode q, p;
	tryagain:
	backward = 0;
	q = text_[np.tl].str()[np.ad];
	switch( q )
	{
		case '(': p=')'; backward=0; break;
		case '[': p=']'; backward=0; break;
		case '{': p='}'; backward=0; break;
		case '<': p='>'; backward=0; break;

		case ')': p='('; backward=1; break;
		case ']': p='['; backward=1; break;
		case '}': p='{'; backward=1; break;
		case '>': p='<'; backward=1; break;
		default:
			if( !backside )
			{	// Check if the parenthesis is at the backside of dp.
				np = leftOf(dp, false);
				backside=1;
				goto tryagain; // Try again!
			}
			// No brace to match
			return dp;
	}

	ulong match;
	unicode b;
	if( !backward )
	{
		match = 1; // We already have 1 opening parenthesis
		while( b = findNextBrace( np, q, p ) ) // q='(' p=')'
		{
			// Keep track of opening/closing parenthesis
			match += 2*(b==q) - 1; // b=='(' ++ else --
			if( match == 0 ) // same amount of () => done!
				break;
		}
	}
	else
	{
		match = 1; // We already have 1 closing parenthesis
		while( b = findPrevBrace( np, q, p ) ) // q=')' p='('
		{
			// Keep track of opening/closing parenthesis
			match += 2*(b==q) - 1; // b==')' ++ else --
			if( match == 0 ) // same amount of () => done!
				break;
		}
	}

	if( match == 0 )
	{
		if( !backside ) // |(abc) => (abc)| (abc|) => (|abc)
			return rightOf(np, false);
		return np;
	}

	return dp;
}

unicode Document::findNextBrace( DPos &dp, unicode q, unicode p ) const
{
	// Loop forward word by word to find the next brace
	// specified in the brace parameter.
	DPos np;

	while( (np=rightOf( dp , true )) > dp )
	{
		dp = np;
		unicode ch = text_[dp.tl].str()[dp.ad];
		if( ch == q || ch == p )
			return ch;
	}
	return L'\0';
}

unicode Document::findPrevBrace( DPos &dp, unicode q, unicode p ) const
{
	// Loop back word by word to find the previous brace,
	// specified in the brace parameters.
	DPos np;
	while( (np=leftOf( dp , true )) < dp )
	{
		dp = np;
		unicode ch = text_[dp.tl].str()[dp.ad];
		if( ch == q || ch == p )
			return ch;
	}
	return L'\0';
}

//-------------------------------------------------------------------------
// 挿入・削除等の作業用関数群, A set of functions for working with ins, del, etc.
//-------------------------------------------------------------------------

ulong Document::getRangeLength( const DPos& s, const DPos& e )
{
	// とりあえず全部足す, Just add it all up.
	ulong ans=0, tl=s.tl, te=e.tl;
	for( ; tl<=te; ++tl )
		ans += len(tl);
	// 先頭行の分を引く, subtract the first line
	ans -= s.ad;
	// 最終行の分を引く, Subtract the last line
	ans -= len(te) - e.ad;
	// 改行コード(CRLF)の分を加える, Add the portion of the line feed code (CRLF)
	ans += (e.tl-s.tl) * 2;
	// おしまい, The end
	return ans;
}

void Document::getText( unicode* buf, const DPos& s, const DPos& e )
{
	if( !buf ) return;
	if( s.tl == e.tl )
	{
		// 一行だけの場合, If you have only one line
		text_[s.tl].CopyAt( s.ad, e.ad-s.ad, buf );
		buf[e.ad-s.ad] = L'\0';
	}
	else
	{
		// 先頭行の後ろをコピー, Copy the end of the first line
		buf += text_[s.tl].CopyToTail( s.ad, buf );
		*buf++ = L'\r', *buf++ = L'\n';
		// 途中をコピー, Copy the middle
		for( ulong i=s.tl+1; i<e.tl; i++ )
		{
			buf += text_[i].CopyToTail( 0, buf );
			*buf++ = L'\r', *buf++ = L'\n';
		}
		// 終了行の先頭をコピー, Copy the beginning of the end line
		buf += text_[e.tl].CopyAt( 0, e.ad, buf );
		*buf = L'\0';
	}
}

void Document::CorrectPos( DPos& pos )
{
	// 正常範囲に収まるように修正, Fix to fall within normal range.
	pos.tl = Min( pos.tl, tln()-1 );
	pos.ad = Min( pos.ad, len(pos.tl) );
}

void Document::CorrectPos( DPos& s, DPos& e )
{
	// 必ずs<=eになるように修正,  Fix it so that s<=e.
	if( s > e )
	{
		ulong t;
		t=s.ad, s.ad=e.ad, e.ad=t;
		t=s.tl, s.tl=e.tl, e.tl=t;
	}
}

bool Document::DeletingOperation
	( DPos& s, DPos& e, unicode*& undobuf, ulong& undosiz )
{
	AutoLock lk( this );

	// 位置補正, Position correction
	CorrectPos( s );
	CorrectPos( e );
	CorrectPos( s, e );

	// 削除される量をカウント, Amount to be deleted
	undosiz = getRangeLength( s, e );

	// Undo操作用バッファ確保, Allocate buffer for Undo operation
	undobuf = reinterpret_cast<unicode*>( mem().Alloc( (undosiz+1)*2 ) );
	if( undobuf )
	{ // We got enough memory...
		getText( undobuf, s, e );// get text to del for undo
	}

	// 削除る, delete
	if( s.tl == e.tl )
	{
		// 一行内削除, line-by-line deletion
		text_[s.tl].RemoveAt( s.ad, e.ad-s.ad );
	}
	else
	{
		// 先頭行の後ろを削除, Remove the end of the first line
		text_[s.tl].RemoveToTail( s.ad );
		// 終了行の残り部分をくっつける, Attach the rest of the end line
		text_[s.tl].InsertToTail( tl(e.tl)+e.ad, len(e.tl)-e.ad );
		// いらん行を削除, delete a line that doesn't belong
		text_.RemoveAt( s.tl+1, e.tl-s.tl );
	}

	// 再解析, Reanalysis
	return ReParse( s.tl, s.tl );
}

bool Document::InsertingOperation
	( DPos& s, const unicode* str, ulong len, DPos& e )
{
	AutoLock lk( this );

	// 位置補正, Position correction
	CorrectPos( s );

	// よーい、どん！, All right, Go!
	e.ad = s.ad;
	e.tl = s.tl;

	// 指定文字列を改行で切り分ける準備
	// Prepare to separate the specified string with a newline.
	const unicode* lineStr;
	ulong lineLen;
	UniReader r( str, len, &lineStr, &lineLen );

	// 一行目…, The first line...
	r.getLine();
	text_[e.tl].InsertAt( e.ad, lineStr, lineLen );
	e.ad += lineLen;

	if( !r.isEmpty() )
	{
		// 二行目～最終行, Second to last line
		do
		{
			r.getLine();
			Line *nline = new Line(lineStr, lineLen);
			if( nline ) text_.InsertAt( ++e.tl, nline );
		} while( !r.isEmpty() );

		// 一行目の最後尾に残ってた文字列を最終行へ
		// Move the remaining string from the end
		// of the first line to the last line.
		Line& fl = text_[s.tl];
		Line& ed = text_[e.tl];
		const ulong ln = fl.size()-e.ad;
		if( ln )
		{
			ed.InsertToTail( fl.str()+e.ad, ln );
			fl.RemoveToTail( e.ad );
		}

		// 終了位置記録, Record end position
		e.ad = lineLen;
	}

	// 再解析, Reanalysis
	return ReParse( s.tl, e.tl );
}



//-------------------------------------------------------------------------
// 挿入コマンド, Insert command
//-------------------------------------------------------------------------

Insert::Insert( const DPos& s, const unicode* str, ulong len, bool del )
	: stt_( s )
	, buf_( str )
	, len_( len )
	, del_( del )
{
}

Insert::~Insert()
{
	if( del_ )
		mem().DeAlloc( const_cast<unicode*>(buf_), (len_+1)*2 );
}

Command* Insert::operator()( Document& d ) const
{
	// 挿入, Insertion
	DPos s=stt_, e;
	bool aa = d.InsertingOperation( s, buf_, len_, e );

	// イベント発火, Event firing
	d.Fire_TEXTUPDATE( s, s, e, aa, true );

	// 逆操作オブジェクトを返す, Return the reverse operation object
	return new Delete( s, e );
}



//-------------------------------------------------------------------------
// 削除コマンド, delete command
//-------------------------------------------------------------------------

Delete::Delete( const DPos& s, const DPos& e )
	: stt_( s )
	, end_( e )
{
}

Command* Delete::operator()( Document& d ) const
{
	// 削除, Deletion
	unicode* buf;
	ulong    siz;
	DPos s = stt_, e=end_;
	bool aa = d.DeletingOperation( s, e, buf, siz );

	// イベント発火, Event firing
	d.Fire_TEXTUPDATE( s, e, s, aa, true );

	// 逆操作オブジェクトを返す, Return the reverse operation object
	Insert *rev = NULL;
	if( buf != NULL )
	{	// Make reverse operation...
		rev = new Insert( s, buf, siz, true );
		if( !rev ) // Clear Undo buffer on failure
			mem().DeAlloc( buf, (siz+1)*2 );
	}
	return rev;
}



//-------------------------------------------------------------------------
// 置換コマンド, Replace command
//-------------------------------------------------------------------------

Replace::Replace(
	const DPos& s, const DPos& e, const unicode* str, ulong len, bool del )
	: stt_( s )
	, end_( e )
	, buf_( str )
	, len_( len )
	, del_( del )
{
}

Replace::~Replace()
{
	if( del_ )
		mem().DeAlloc( const_cast<unicode*>(buf_), (len_+1)*2 );
}

Command* Replace::operator()( Document& d ) const
{
	// 削除, Deletion
	unicode* buf;
	ulong    siz;
	DPos s=stt_, e=end_;
	bool aa = d.DeletingOperation( s, e, buf, siz );

	// 挿入, Insertion
	DPos e2;
	aa = (d.InsertingOperation( s, buf_, len_, e2 ) || aa);

	// イベント発火, event firing
	d.Fire_TEXTUPDATE( s, e, e2, aa, true );

	// 逆操作オブジェクトを返す, Return the reverse operation object
	Replace *rev = NULL;
	if( buf )
	{	// Make reverse operation...
		rev = new Replace( s, e2, buf, siz, true );
		if( !rev ) // Clear Undo buffer on failure
			mem().DeAlloc( buf, (siz+1)*2 );
	}
	return rev;
}



//-------------------------------------------------------------------------
// マクロコマンド, macro command
//-------------------------------------------------------------------------

Command* MacroCommand::operator()( Document& doc ) const
{
	doc.setBusyFlag(true);

	MacroCommand* undo = new MacroCommand;
	if( !undo ) return NULL;
	undo->arr_.ForceSize( size() );

//	doc.acc_Fire_TEXTUPDATE_begin();
	for( ulong i=0,e=arr_.size(); i<e; ++i )
		undo->arr_[e-i-1] = (*arr_[i])(doc);
//	doc.acc_Fire_TEXTUPDATE_end();

	doc.setBusyFlag(false);
	return undo;
}



//-------------------------------------------------------------------------
// ファイルに保存, Save to file
//-------------------------------------------------------------------------

void Document::SaveFile( ki::TextFileW& tf )
{
	urdo_.SavedHere();
	for( ulong i=0,iLast=tln()-1; i<=iLast; ++i )
		tf.WriteLine( tl(i), len(i), i==iLast );
}



//-------------------------------------------------------------------------
// バッファの内容を全部破棄（暫定）, Destroy all buffer contents (temporary)
//-------------------------------------------------------------------------

void Document::ClearAll()
{
	// 全部削除, delete everything
	Execute( Delete( DPos(0,0), DPos(0xffffffff,0xffffffff) ) );

	// Undo-Redoチェインをクリア
	// Clear the Undo-Redo chain
	urdo_.Clear();
	urdo_.SavedHere();
	Fire_MODIFYFLAGCHANGE();
}



//-------------------------------------------------------------------------
// ファイルを開く（暫定）, Open a file (temporary)
//-------------------------------------------------------------------------

void Document::OpenFile( TextFileR& tf )
{
	// ToDo: マルチスレッド化, ToDo: multi-threaded
	//currentOpeningFile_ = tf;
	//thd().Run( *this );
	// 挿入, Insertion
	DPos e(0,0);

//	// Quick UTF16 special direct reading.
//	if( tf.codepage() == UTF16LE ||  tf.codepage() == UTF16l )
//	{
//		DPos p(0,0);
//		//DWORD otime = GetTickCount();
//		const unicode *buf = reinterpret_cast<const unicode*>( tf.rawData() );
//		int bom = *buf == 0xfffe; // Skip BOM
//		InsertingOperation(p , buf+bom, (tf.size()/2)-bom, e );
//		Fire_TEXTUPDATE( DPos(0,0), DPos(0,0), e, true, false );
//		//MessageBox(GetForegroundWindow(),  SInt2Str(GetTickCount()-otime).c_str(), TEXT("Time in ms UTF16:"), 0);
//		return; // DONE!
//	}

	// Super small stack buffer in case the malloc fails
	#define SBUF_SZ 1800
	unicode sbuf[SBUF_SZ];

	// Use big buffer (much faster on long lines)
#ifdef WIN64
	size_t buf_sz = 2097152;
	// static unicode buf[2097152]; // 4MB on x64
#else
	// static unicode buf[131072]; // 256KB on i386
	size_t buf_sz = 131072;
#endif
	// Do not allocate more mem than twice the file size in bytes.
	// Should help with loaing small files on Win32s.
	buf_sz = Min( buf_sz, (size_t)(tf.size()+16)<<1 );
	unicode *buf=NULL;
	if( buf_sz > SBUF_SZ )
		buf = new unicode[buf_sz];
	if( !buf )
	{
		buf = sbuf;
		buf_sz = SBUF_SZ;
	}
//	DWORD otime = GetTickCount();
	size_t L;
	for( ulong i=0; L = tf.ReadBuf( buf, buf_sz ); )
	{
		DPos p(i,0xffffffff);
		InsertingOperation( p, buf, (ulong)L, e );
		i = tln() - 1;
	}

	if( buf != sbuf )
		delete [] buf;

//	MessageBox(GetForegroundWindow(),  SInt2Str(GetTickCount()-otime).c_str(), TEXT("Time in ms:"), 0);
	// イベント発火, Event firing
	Fire_TEXTUPDATE( DPos(0,0), DPos(0,0), e, true, false );
}


void Document::StartThread()
{
/*
	// ToDo:
	aptr<TextFileR> tf = currentOpeningFile_;

	// 挿入
	unicode buf[1024];
	while( !isExitRequested() && tf->state() )
	{
		ulong i, j;
		DPos s(i=tln()-1, j=len(i)), e;
		if( ulong L = tf->ReadLine( buf, countof(buf) ) )
			InsertingOperation( s, buf, L, e );
		if( tf->state() == 1 )
			InsertingOperation( DPos(i,0xffffffff), L"\n", 1, e );

		// イベント発火
		Fire_TEXTUPDATE( s, s, e, true, false );
	}
*/
}

