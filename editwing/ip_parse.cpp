
#include "../kilib/stdafx.h"
#include "ip_doc.h"
using namespace ki;
using namespace editwing;
using namespace editwing::doc;



//=========================================================================
//---- ip_parse.cpp キーワード解析
//
//		キーワード定義ファイルに従って、保持する文字列を
//		適切に切り分ける作業がここ。
//
//---- ip_text.cpp   文字列操作・他
//---- ip_wrap.cpp   折り返し
//---- ip_scroll.cpp スクロール
//---- ip_draw.cpp   描画・他
//---- ip_cursor.cpp カーソルコントロール
//=========================================================================



//=========================================================================
//
// 解析結果データ仕様
// これだけ色々姑息な手段を持ち込んで本当に
// 速くなっているのかどうかは不明…(^^;
//
// -----------------------------------------------
//
// Line::isLineHeadCommented_
//    0: 行頭がブロックコメントの内部ではない, The beginning of the line is not inside the block comment
//    1: 行頭がブロックコメントの内部,
//
// -----------------------------------------------
//
// Line::commentTransition_
//   00: 行末は常にコメントの外, End of line always out of comment
//   01: 行頭と行末はコメント状態が逆転, Comment state is reversed at the beginning of a line and at the end of a line
//   10: 行頭と行末はコメント状態が同じ, Comment state is the same at the beginning of a line and at the end of a line
//   11: 行末は常にコメントの中, The end of the line is always in the comment
//
// -----------------------------------------------
//
// 以上二つのフラグを元に、前の行の情報から今の行の情報を
//   this.head = (prev.trans >> prev.head)&1;
// で順次計算していくことが出来る。
// この計算の際に内部バッファの状態まで書き換えるのは
// コストがでかすぎるので、次に示すフラグを見ながら
// 描画寸前に適宜調整する。
//
// -----------------------------------------------
//
// Line::commentBitReady_
//   コメントビットが調整済みかどうか
//   Whether the comment bits have been adjusted or not
//
// -----------------------------------------------
//
// Line::str_[]
//   UCS-2ベタで、文字列データがそのまま格納される。
//   ただし、パーサの高速化のために最終文字の後ろに
//   0x007fが付加される。
//   UCS-2 solid, string data is stored as is.
//   However, to speed up the parser, the last character is followed by
//   0x007f is appended.
//
// -----------------------------------------------
//
// Line::flg_
//   一文字毎に、下のような8bitのフラグを割り当てる
//   Assign an 8-bit flag as shown below for each character
//   | aaabbbcd |
//
// -----------------------------------------------
//
// aaa == "PosInToken"
//     0: トークンの途中, Token on the way
//   1-6: トークンの頭。次の頭は1-6文字先。, Token head. The next head is 1-6 characters ahead.
//     7: トークンの頭。次の頭は7文字以上先。, Token head. The next head is at least 7 characters ahead.
//
// -----------------------------------------------
//
// bbb == "TokenType"
//     0: TAB: タブ文字, tab cahar
//     1: WSP: ホワイトスペース, white space char
//     2: TXT: 普通の文字, normal text
//     3:  CE: コメント開始タグ, comment-start tag
//     4:  CB: コメント終了タグ, end-of-comment tag
//     5:  LB: 行コメント開始タグ, line comment start tag
//     6:  Q1: '' 引用符1, Sinlge Quote
//     7:  Q2: "" 引用符2, Double Quote
//
// -----------------------------------------------
//
//  c  == "isKeyword?"
//     0: キーワードではない, Not a keyword.
//     1: キーワード, Keyword
//
// -----------------------------------------------
//
//  d  == "inComment?"
//     0: コメントの中ではない, not in a comment
//     1: コメントの中, in a comment
//
// -----------------------------------------------



namespace {
//-------------------------------------------------------------------------
// コメントの中なのか外なのか等を判定するためのオートマトン
//
// /* が出たらその後ろはコメントで */ が出たらその後ろはノーマルゾーン
// …という単純な規則では上手く行かない。例えば str"/*"str なんてものが
// 出現した場合に困ってしまう。そこで、
//   ・普通のテキスト
//   ・ブロックコメントの中
//   ・行コメントの中
//   ・一重引用符の中
//   ・二重引用符の中
// の５種類の状態に分けて、それぞれの場合について、どの記号が出たら
// 次にどの状態に移るのか…を処理する必要がある。その状態変化の規則を
// 5x5の２次元配列で与えて管理する。
//-------------------------------------------------------------------------

enum CommentDFASymbol{ sCE, sCB, sLB, sQ1, sQ2, sXXX };
struct CommentDFA
{
	// <状態>
	// 最下位bitが、現在コメント内かどうかのフラグになります。
	// ブロックコメント中かどうかは (state>>1)&(state) で。
	//   000: normal text        011: in BlockComment
	//   001: in LineComment     100: in Quote2
	//   010: in Quote1
	//
	// <シンボル>
	// C++で言うと下の通り
	// 値はTokenTypeフラグとシンクロするようになってます。
	//   000: CE */              011: Q1 '
	//   001: CB /*              100: Q2 "
	//   010: LB //

	// 初期状態を指定。コメント内かコメント外か
	CommentDFA( bool inComment )
		: state( inComment ? 3 : 0 ) {}

	// 入力符号を与えて状態遷移
	void transit( uchar sym )
		{ state = tr_table[state][sym]; }

	static void SetCEequalCB(bool set)
	{
		// Is CE == CB then we must go from
		// iBc -> Ntx when we see CB
		tr_table[/*iBc*/3][/*QB*/1] = set? 0: 3;
	}

	// 現在の状態
	uchar state;

	// 状態遷移テーブル
	static uchar tr_table[5][5];
};

uchar CommentDFA::tr_table[5][5] = {
// state                  // CE,  CB,  LB,  Q1,  Q2
/* 000 Ntx */{0,3,1,2,4}, // Ntx, iBc, iLc, iQ1, iQ2
/* 001 iLc */{1,1,1,1,1}, // iLc, iLc, iLc, iLc, iLc
/* 010 iQ1 */{2,2,2,0,2}, // iQ1, iQ1, iQ1, Ntx, iQ1
/* 011 iBc */{0,3,3,3,3}, // Ntx,    , iBc, iBc, iBc
/* 100 iQ2 */{4,4,4,4,0}, // iQ2, iQ2, iQ2, iQ2, Ntx
};



//-------------------------------------------------------------------------
// 単純な、キーワード格納構造体。
// ChainHashの要素にするためnextポインタがつけてあります。
// DO NOT USE new/delete for Keyword, stick to Keyword::New
//-------------------------------------------------------------------------
struct Keyword
{
	ushort      next;
	ushort      len;
	unicode     str[1];

	static Keyword *New(Arena *ar,  const unicode *s, size_t ll )
	{
		ushort l = static_cast<ushort>(ll);
		Keyword *x = reinterpret_cast<Keyword *>( ar->alloc( (sizeof(Keyword) + l * sizeof(unicode)) ) );
		if( x )
		{
			x->next = 0;
			x->len  = l;
			memmove(x->str, s, l*sizeof(unicode));
			x->str[l] = L'\0';
		}
		//else MessageBox(NULL, s, NULL, 0);
		//LOGGERF( TEXT("sizeof(Keyword) = %d, remaining = %d bytes"),  sizeof(Keyword), (UINT)(ar->end - ar->sta));
		return x;
	}
};



//-------------------------------------------------------------------------
// サポート関数。Unicodeテキスト同士の比較
//-------------------------------------------------------------------------

static bool compare_s(const unicode* a,const unicode* b, size_t l)
{
	// 大文字小文字を区別, Case sensitive
	while( l-- )
		if( *a++ != *b++ )
			return false;
	return true;
}

static bool compare_i(const unicode* a,const unicode* b,size_t l)
{
	// 大文字小文字を区別しない（雑）, Case insensitive (misc)
	while( l-- )
		if( ((*a++) ^ (*b++)) & 0xdf )
			return false;
	return true;
}



//-------------------------------------------------------------------------
// 与えられた記号文字列から、コメント開始等の意味のあるトークンを
// 切り出してくるための構造。
// meaningful tokens from a given symbol string, such as the start of a comment.
// Structure to cut out.
//-------------------------------------------------------------------------

class TagMap
{
	Keyword* tag_[3]; // 0:CE 1:CB 2:LB
	bool esc_, q1_, q2_, map_[768]; // 128
	BYTE arbuf[64];
	Arena ar;

public:

	TagMap( const unicode* cb, size_t cblen,
		    const unicode* ce, size_t celen,
		    const unicode* lb, size_t lblen,
		    bool q1, bool q2, bool esc )
		: esc_( esc )
		, q1_ ( q1 )
		, q2_ ( q2 )
		, ar ( arbuf, sizeof(arbuf) )
	{
		// '/' で始まる記号は使われているか…？
		// みたいな、１文字目のみのチェックに使う表を作成
		tag_[0] = tag_[1] = tag_[2] = NULL;
		mem00( map_, sizeof(map_) );
		map_[L'\''] = q1;
		map_[L'\"'] = q2;
		map_[L'\\'] = esc;
		if( celen!=0 && *ce < 0x80 ){ map_[*ce]=true; tag_[0]=Keyword::New(&ar, ce,celen); }
		if( cblen!=0 && *cb < 0x80 ){ map_[*cb]=true; tag_[1]=Keyword::New(&ar, cb,cblen); }
		if( lblen!=0 && *lb < 0x80 ){ map_[*lb]=true; tag_[2]=Keyword::New(&ar, lb,lblen); }
	}

//	~TagMap()
//	{
//		// キーワード解放,
//		Keyword::Delete( tag_[0] );
//		Keyword::Delete( tag_[1] );
//		Keyword::Delete( tag_[2] );
//	}

	bool does_esc()
	{
		// \ によるエスケープをするかどうか
		return esc_;
	}

	ulong SymbolLoop(
		const unicode* str, ulong len, ulong& mlen, uchar& sym )
	{
		// 有意味な記号にマッチするまでループ
		// 返値に、マッチするまでに飛ばした文字数、
		// mlen,symに、マッチした記号の情報を返す
		// Loop until a meaningful symbol is matched.
		// Return value is the number of characters skipped before the match.
		// And the matched length/symbol are copied in mlen and sym.

		ulong ans=0;
		for( sym=sXXX, mlen=1; ans<len; ++ans )
		{
			if( map_[str[ans]] )
			{
				for( int i=2; i>=0; --i )
				{
					if( tag_[i]!=NULL
					 && tag_[i]->len <= len-ans
					 && compare_s(
						tag_[i]->str, str+ans, tag_[i]->len ) )
					{
						sym  = i;
						mlen = tag_[i]->len;
						goto symbolfound;
					}
				}

				if( str[ans] == L'\'' ) // 一重引用符 - single quote
				{
					if( q1_ )
					{
						sym  = sQ1;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\"' ) // 二重引用符 - double quote
				{
					if( q2_ )
					{
						sym  = sQ2;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\\' ) // \ の後の文字はSkip
				{
					if( esc_ && ans+1<len )
						++ans;
				}
			}
		}

	symbolfound:
		return ans;
	}
};



//-------------------------------------------------------------------------
// 与えられた文字列がキーワードかどうか高速判定するためのハッシュ表
// Hash table for fast determination of whether a given string is a keyword
//-------------------------------------------------------------------------
class KeywordMap
{
	// Should be a power of two!
	enum { HTABLE_SIZE = 2048 };
	ushort   backet_[HTABLE_SIZE];
	Arena ar;
	size_t elems_;
	bool (*compare_)(const unicode*,const unicode*,size_t);
	uint  (*hash)( const unicode* a, size_t al );
public:

	KeywordMap( bool bCaseSensitive )
		: ar ( NULL, 0 )
		, elems_ ( 0 )
		, compare_( bCaseSensitive ? compare_s : compare_i )
		, hash    ( bCaseSensitive ? hash_s : hash_i )
	{
		// ハッシュ表初期化
		mem00( backet_, sizeof(backet_) );
	}

	void SetArenaBufSize( size_t count )
	{
		if (count > 65536)
			count = 65536; // LIMIT
		BYTE *buf = NULL;
		if(count != 0)
			buf = (BYTE*)malloc(count);

		BYTE *obuf = ar.sta;
		ar = Arena(buf, count);

		free( obuf );
	}

	~KeywordMap()
	{
		// 解放
		free( ar.sta );
//	#ifdef _DEBUG
//		if( elems_ )
//		{
//			LOGGER( "KEYWORD HASH MAP:" );
//			for( size_t i =0; i < countof(backet_); i++ )
//				LOGGERF( TEXT("%lu"), (DWORD)backet_[i] );
//		}
//	#endif
	}
	#define KW(a) ((Keyword*)(ar.sta + a))

	void AddKeyword( const unicode* str, size_t len )
	{
		// データ登録
		ushort x = (ushort)((BYTE*)Keyword::New(&ar, str,len) - ar.sta);
		int      h = hash(str,len);

		if( backet_[h] == 0 )
		{
			// ハッシュテーブルが空の場合, Hash table slot is free.
			backet_[h] = x;
		}
		else
		{
			// チェイン末尾に繋ぐ場合, chain to the existing element
			//MessageBoxW(NULL, backet_[h]->str, x->str , MB_OK);
			ushort q=backet_[h], p=KW(q)->next;
			while( p!=0 )
				q=p, p=KW(q)->next;

			KW(q)->next = x;
		}

		// データクリア用のリストにも入れておく
		//dustbox_.Add(x);
		++elems_;
	}

	uchar inline isKeyword( const unicode* str, size_t len ) const
	{
		// 登録されているキーワードと一致するか？
		if( elems_ ) // Nothing to do for empty keyword list.
			for( ushort p=backet_[hash(str,len)]; p!=0; p=KW(p)->next )
				if( KW(p)->len==len && compare_( KW(p)->str, str, len ) )
					return 2; // We must set the c bit of aaabbbcd
		return 0;
	}
	#undef KW
private:

	static uint hash_i( const unicode* a, size_t al )
	{
		// 12bitに潰すめっちゃ雑なハッシュ関数
		// ルーチン分けるの面倒なので、大文字小文字は常に区別されない。(^^;
		// Very messy hash function that collapses to 12 bits.
		// case-insensitive.
		uint h=0,i=0;
		while( al-- )
		{
			h ^= ((*(a++)&0xdf)<<i);
			i = (i+5)&7;
		}
		return h&(HTABLE_SIZE-1);
	}

	static uint hash_s( const unicode* a, size_t al )
	{
		// case-sensitive
		uint h=0,i=0;
		while( al-- )
		{
			h ^= (*a++)<<i;
			i = (i+5)&7;
		}
		return h&(HTABLE_SIZE-1);
	}
};



//-------------------------------------------------------------------------
// 以上の道具立てでもって、テキストの解析を行うParser
//-------------------------------------------------------------------------
}
class editwing::doc::Parser
{
public:
	KeywordMap kwd_;
	TagMap     tag_;

public:
	// 初期化１
	Parser(
		const unicode* cb, size_t cblen,
		const unicode* ce, size_t celen,
		const unicode* lb, size_t lblen,
		bool q1, bool q2, bool esc,
		bool casesensitive
	)
		: kwd_( casesensitive )
		, tag_( cb, cblen, ce, celen, lb, lblen, q1, q2, esc )
	{
		if( cb && ce ) // In case begin and end comment strings are the same i.e.: Python
			CommentDFA::SetCEequalCB( cblen==celen && !my_lstrncmpW(ce, cb, cblen) );
	}

	//void Trim() { kwd_.Trim(); }
	void SetKeywordArenaSize(size_t count)
	{
		kwd_.SetArenaBufSize(count);
	}

	// 初期化２：キーワード追加
	void AddKeyword( const unicode* str, size_t len )
	{
		kwd_.AddKeyword( str, len );
	}

	// 行データ解析
	uchar Parse( Line& line, uchar cmst )
	{
		line.TransitCmt( cmst );

		// ASCII振り分けテーブル。
		// シフト無しでTokenTypeに流用出来るようにするため、
		// 値が４飛びになってます
		// We want to separate the tabs from other non printable characters.
		enum { T=1, W=4, A=8, S=12, O=0 };
		static const uchar letter_type[768] = {
			O,O,O,O,O,O,O,O,O,T,O,O,O,O,O,O, // NULL-SHI_IN
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, // DLE-US
			W,S,S,S,S,S,S,S,S,S,S,S,S,S,S,S, //  !"#$%&'()*+,-./
			A,A,A,A,A,A,A,A,A,A,S,S,S,S,S,S, // 0123456789:;<=>?
			S,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // @ABCDEFGHIJKLMNO
			A,A,A,A,A,A,A,A,A,A,A,S,S,S,S,A, // PQRSTUVWXYZ[\]^_
			S,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // `abcdefghijklmno
			A,A,A,A,A,A,A,A,A,A,A,S,S,S,S,O, // pqrstuvwxyz{|}~
			// Latin-1 Supplement (0x0080-0x00FF)
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, // C1 Controls
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O, // C1 Controls
			S,A,A,A,A,A,S,A,A,A,A,S,A,A,A,S, //  !¢£?\|§¨ca≪¬[SHY-]R￣
			A,S,A,A,A,A,S,S,A,S,A,S,A,A,A,S, // °±23´μ¶・，1o≫????
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // AAAAAAACEEEEIIII
			A,A,A,A,A,A,A,S,A,A,A,A,A,A,A,A, // DNOOOOO×OUUUUYTs
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A, // aaaaaaaceeeeiiii
			A,A,A,A,A,A,A,S,A,A,A,A,A,A,A,A, // dnooooo÷ouuuuyty
			// Latin Extended-A (0x0100-0x017f)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			// Latin Extended-B (0x0180-0x024f)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			// IPA Extensions (0x0250-0x02af)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			// Spacing Modifier Letters (0x02b0-0x02ff)
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
		};

		// PosInToken算出用の距離エンコーダ( 5bitシフト済み )
		// Distance encoder for PosInToken calculation ( 5bit shifted )
		//  ( _d>7 ? 7<<5 : _d<<5 )
		#define tkenc(_d) ( (_d)>7 ? 0xe0 : (_d)<<5 )

		// コメント状態遷移追跡用オートマトン
		CommentDFA dfa[2] = {CommentDFA(false), CommentDFA(true)};
		const uchar& cmtState  = dfa[line.isLineHeadCmt()].state;
		uchar commentbit = cmtState&1;

		// 作業領域, workspace
		uchar sym;
		ulong j, k, um, m;
		uchar t, f;

		// ループ〜
		const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong           ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			j = i;

			// ASCII文字でない場合 (Not ASCII char)
			// was 0x007f (ASCII) I replaced by 0x02ff
			// that corresponds to all extended latin charatcter.
			// This is needed if yu want to ctrl+select words that
			// have a mix of ASCII and other LATIN characters.
			if( str[i] > 0x02ff ) // Non latin
			{
				f = (ALP | commentbit);
				if( str[i] == 0x3000 )//L'　' )
					while( str[++j] == 0x3000 )
						flg[j] = f;
				else
					while( str[++j] >= 0x02ff && str[j]!=0x3000 )
						flg[j] = f;
				flg[i] = static_cast<uchar>(tkenc(j-i) | f);
			}
			// ASCII文字の場合?? (ASCII char?)
			// All latin chars up to IPA Extensions 0x0000-0x02ff
			else
			{
				t = letter_type[str[i]];
				if( t==S && tag_.does_esc() )
					do
						if( j+1<ie && str[j]==L'\\' )
							j++;
					while( str[++j]<0x7f && S==letter_type[str[j]] );
				else
					while( ++j<ie && str[j]<0x02ff && t==letter_type[str[j]] );

				f = (t | commentbit);

				switch( t ) // letter type:
				{
				// アルファベット＆数字, Alphabets & Numbers (a-Z, 0-9)
				case A:
					if( str[i] < 0x007f ) // ASCII only
						f |= kwd_.isKeyword( str+i, j-i );
					// fall...

				// CTL characters (0-32 + 127-160)
				case O:
				// タブ・制御文字, Tabs
				case T:
					// fall...

				// 半角空白 (space, 32)
				case W:
					for( k=i+1; k<j; ++k )
						flg[k] = f;
					flg[i] = (uchar)(tkenc(j-i)|f);
					break;

				// 記号 (Symbol, 33-47, 58-64, 91-94, 96, 123-126)
				case S:
					k = i;
					while( k < j )
					{
						// マッチしなかった部分, The part that did not match
						um = tag_.SymbolLoop( str+k, j-k, m, sym );
						f = (0x20 | ALP | commentbit);
						while( um-- )
							flg[k++] = f;
						if( k >= j )
							break;

						// マッチした部分, Matched part
						f = (CE | commentbit);
						dfa[0].transit( sym );
						dfa[1].transit( sym );
						commentbit = cmtState&1;
						if( sym != 0 ) // 0:comment end
							f = (((sym+3)<<2) | commentbit);
						flg[k++] = (uchar)(tkenc(m)|f);
						while( --m )
							flg[k++] = f;
					}
					break;
				}
			}
		}

		// transitフラグ更新 Update transit flag
		line.SetTransitFlag(
			(dfa[1].state & (dfa[1].state<<1)) |
			((dfa[0].state>>1) & dfa[0].state)
		);
		line.CommentBitUpdated();
		return line.TransitCmt( cmst );
	}

	// コメントビットを正しく調整
	void SetCommentBit( Line& line )
	{
		CommentDFA dfa( line.isLineHeadCmt()==1 );
		uchar commentbit = dfa.state&1;

		// ループ〜
		// const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong         j,ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			// Tokenの終端を得る, Get the end of the Token
			uchar k = (flg[i]>>5);
			j = i + k;
			if( j >= ie )
				j = ie;
			else if( k==7 ) // || k==0 )
				while( j<ie && (flg[j]>>5)==0  ) // check bound BEFORE [] !!!
					++j;

			k = (flg[i] & 0x1c);
			if( k <= CE )
			{
				for( ; i<j; ++i )
					flg[i] = (uchar)((flg[i] & 0xfe) | commentbit);
			}
			if( k >= CE )
			{
				dfa.transit( (k>>2)-3 );
				commentbit = dfa.state&1;
				if( k != CE )
					for( ; i<j; ++i )
						flg[i] = (uchar)((flg[i] & 0xfe) | commentbit);
			}
		}

		line.CommentBitUpdated();
	}
};


//-------------------------------------------------------------------------
// 定義ファイル読みとり処理とか
//-------------------------------------------------------------------------

Document::Document( )
	: evHanNum_ ( 0 )
	, busy_   ( false )
	, acc_textupdate_mode_ ( false )
	, acc_reparsed_ ( false )
	, acc_nmlcmd_ (false )
{
	text_.Add( Line(L"",0) ); // 最初は一行だけ
	SetKeyword( NULL, 0 );        // キーワード無し
}

Document::~Document()
{
	// このファイルにデストラクタを入れておかないと、
	// delete parser_ が出来なくなる。^^;
}

void Document::SetKeyword( const unicode* defbuf, size_t siz )
{
	// BOMがあったらスキップ
	if( siz!=0 && *defbuf==0xfeff )
		++defbuf, --siz;

	// 読み込み準備
	const unicode* str=NULL;
	size_t       len=0;
	UniReader r( defbuf, siz, &str, &len );
	bool          flags[] = {false,false,false,false};
	const unicode* tags[] = {NULL,NULL,NULL};
	size_t       taglen[] = {0,0,0};
	if( siz != 0 )
	{
		if( *defbuf != L'0' && *defbuf != L'1')
		{
			LOGGER( "Invalid kwd file, (should start with 0 or 1)" );
			return;
		}

		// １行目:フラグ
		//   case? q1? q2? esc?
		r.getLine();
		for( size_t i=0; i<len; ++i )
			flags[i] = (str[i]==L'1');

		// ２〜４行目
		//   ブロコメ開始記号、ブロコメ終了記号、行コメ記号
		// comment start symbol, end symbol line comment symbol
		for( int j=0; j<3; ++j )
		{
			r.getLine();
			  tags[j] = str;
			taglen[j] = len;
		}
	}

	if( taglen[2] )
	{// Copy single line comment string (LB) in a convenient buffer.
		size_t cstrlen=Min(taglen[2], countof(CommentStr_));
		memmove(CommentStr_, tags[2], cstrlen*sizeof(*CommentStr_));
		CommentStr_[cstrlen]='\0'; // be sure to NULL terminate
	}
	else
	{// Default comment string is > when there is no .kwd files.
		CommentStr_[0] = L'>'; CommentStr_[1] = L'\0';
	}


	// パーサー作成
	Parser *prs = new Parser(
		tags[0], taglen[0], tags[1], taglen[1], tags[2], taglen[2],
		flags[1], flags[2], flags[3], flags[0] );
	if( prs )
		parser_.reset( prs );

	if( siz > 0 && !r.isEmpty() )
	{
		// calculate the necessary memory for the keyword list.
		const unicode *b = str;
		size_t n = siz - (str - defbuf), totsz = 0;
		while( n-- )
		{
			enum { align = sizeof(void*) -1 };
			++b;
			if( *b == '\n' )
			{
				if( n && b[1] == '\n' || b[1] == '\r' )
					continue; // Skip extra empty lines
				totsz = (totsz + align + sizeof(Keyword)) & ~(size_t)align;
			}
			else
				totsz += 2;
		}

		parser_->SetKeywordArenaSize( totsz );
		// ５行目以降：キーワードリスト
		while( !r.isEmpty() )
		{
			r.getLine();
			if( len != 0 )
				parser_->AddKeyword( str, len );
		}
	}

	// 全行解析し直し
	//DWORD otime = GetTickCount();
	ReParse( 0, tln()-1 );

	//TCHAR buf[128];
	//wsprintf(buf, TEXT("%lu ms"), GetTickCount() - otime);
	//MessageBox(NULL, NULL, buf, 0);

	// 変更通知
	Fire_KEYWORDCHANGE();
}

bool A_HOT Document::ReParse( ulong s, ulong e )
{
	ulong i;
	uchar cmt = text_[s].isLineHeadCmt();

	// まずは変更範囲を再解析, First, reanalyze the scope of the change
	for( i=s; i<=e; ++i )
		cmt = parser_->Parse( text_[i], cmt );

	// コメントアウト状態に変化がなかったらここでお終い。
	// If there is no change in the commented-out status, we are done here.
	if( i==tln() || text_[i].isLineHeadCmt()==cmt )
		return false;

	// 例えば、/* が入力された場合などは、下の方の行まで
	// コメントアウト状態の変化を伝達する必要がある。
	// For example, if a /* is entered, then down to the bottom line.
	// need to communicate the change in commented-out status.
	do
		cmt = text_[i++].TransitCmt( cmt );
	while( i<tln() && text_[i].isLineHeadCmt()!=cmt );
	return true;
}

void Document::SetCommentBit( const Line& x ) const
{
	parser_->SetCommentBit( const_cast<Line&>(x) );
}
