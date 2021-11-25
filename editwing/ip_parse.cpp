#include "stdafx.h"
#include "ip_doc.h"
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
//    0: 行頭がブロックコメントの内部ではない
//    1: 行頭がブロックコメントの内部
//
// -----------------------------------------------
//
// Line::commentTransition_
//   00: 行末は常にコメントの外
//   01: 行頭と行末はコメント状態が逆転
//   10: 行頭と行末はコメント状態が同じ
//   11: 行末は常にコメントの中
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
//
// -----------------------------------------------
//
// Line::str_[]
//   UCS-2ベタで、文字列データがそのまま格納される。
//   ただし、パーサの高速化のために最終文字の後ろに
//   0x007fが付加される。
//
// -----------------------------------------------
//
// Line::flg_
//   一文字毎に、下のような8bitのフラグを割り当てる
//   | aaabbbcd |
// 
// -----------------------------------------------
//
// aaa == "PosInToken"
//     0: トークンの途中
//   1-6: トークンの頭。次の頭は1-6文字先。
//     7: トークンの頭。次の頭は7文字以上先。
//
// -----------------------------------------------
//
// bbb == "TokenType"
//     0: TAB: タブ文字
//     1: WSP: ホワイトスペース
//     2: TXT: 普通の文字
//     3:  CE: コメント開始タグ
//     4:  CB: コメント終了タグ
//     5:  LB: 行コメント開始タグ
//     6:  Q1: '' 引用符1
//     7:  Q2: "" 引用符2
//
// -----------------------------------------------
//
//  c  == "isKeyword?"
//     0: キーワードではない
//     1: キーワード
//
// -----------------------------------------------
//
//  d  == "inComment?"
//     0: コメントの中ではない
//     1: コメントの中
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

enum CommentDFASymbol{ sCB, sCE, sLB, sQ1, sQ2, sXXX };
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
	void transit( int sym ) 
		{ state = tr_table[state][sym]; }

	// 現在の状態
	int state;

	// 状態遷移テーブル
	static const int tr_table[5][5];
};

const int CommentDFA::tr_table[5][5] = {
	{0,3,1,2,4},
	{1,1,1,1,1},
	{2,2,2,0,2},
	{0,3,3,3,3},
	{4,4,4,4,0},
};



//-------------------------------------------------------------------------
// 単純な、キーワード格納構造体。
// ChainHashの要素にするためnextポインタがつけてあります。
//-------------------------------------------------------------------------

struct Keyword : public Object
{
	unicode*    str;
	const ulong len;
	Keyword*   next;

	Keyword( const unicode* s, ulong l )
		: str( new unicode[l+1] )
		, len( l )
		, next( NULL )
		{ memmove( str, s, l*sizeof(unicode) ); }

	~Keyword()
		{ delete [] str; }
};



//-------------------------------------------------------------------------
// サポート関数。Unicodeテキスト同士の比較
//-------------------------------------------------------------------------

static bool compare_s(const unicode* a,const unicode* b,ulong l)
{
	// 大文字小文字を区別
	while( l-- )
		if( *a++ != *b++ )
			return false;
	return true;
}

static bool compare_i(const unicode* a,const unicode* b,ulong l)
{
	// 大文字小文字を区別しない（雑）
	while( l-- )
		if( ((*a++) ^ (*b++)) & 0xdf )
			return false;
	return true;
}



//-------------------------------------------------------------------------
// 与えられた記号文字列から、コメント開始等の意味のあるトークンを
// 切り出してくるための構造。
//-------------------------------------------------------------------------

class TagMap
{
	Keyword* tag_[3]; // 0:CE 1:CB 2:LB
	bool esc_, q1_, q2_, map_[128];

public:

	TagMap( const unicode* cb, ulong cblen,
		    const unicode* ce, ulong celen,
		    const unicode* lb, ulong lblen,
		    bool q1, bool q2, bool esc )
		: q1_ ( q1 )
		, q2_ ( q2 )
		, esc_( esc )
	{
		// '/' で始まる記号は使われているか…？
		// みたいな、１文字目のみのチェックに使う表を作成
		tag_[0] = tag_[1] = tag_[2] = NULL;
		mem00( map_, sizeof(map_) );
		map_[L'\''] = q1;
		map_[L'\"'] = q2;
		map_[L'\\'] = esc;
		if( celen!=0 ){ map_[*ce]=true; tag_[0]=new Keyword(ce,celen); }
		if( cblen!=0 ){ map_[*cb]=true; tag_[1]=new Keyword(cb,cblen); }
		if( lblen!=0 ){ map_[*lb]=true; tag_[2]=new Keyword(lb,lblen); }
	}

	~TagMap()
	{
		// キーワード解放
		delete tag_[0];
		delete tag_[1];
		delete tag_[2];
	}

	bool does_esc()
	{
		// \ によるエスケープをするかどうか
		return esc_;
	}

	ulong SymbolLoop(
		const unicode* str, ulong len, ulong& mlen, int& sym )
	{
		// 有意味な記号にマッチするまでループ
		// 返値に、マッチするまでに飛ばした文字数、
		// mlen,symに、マッチした記号の情報を返す

		int i;
		ulong ans=0;
		for( sym=sXXX, mlen=1; ans<len; ++ans )
			if( map_[str[ans]] )
			{
				for( i=2; i>=0; --i )
					if( tag_[i]!=NULL
					 && tag_[i]->len <= len-ans
					 && compare_s(
						tag_[i]->str, str+ans, tag_[i]->len ) )
					{
						sym  = i;
						mlen = tag_[i]->len;
						goto symbolfound;
					}
				if( str[ans] == L'\'' ) // 一重引用符
				{
					if( q1_ )
					{
						sym  = sQ1;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\"' ) // 二重引用符
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

	symbolfound:
		return ans;
	}
};



//-------------------------------------------------------------------------
// 与えられた文字列がキーワードかどうか高速判定するためのハッシュ表
//-------------------------------------------------------------------------

class KeywordMap
{
	Keyword*          backet_[4096];
	storage<Keyword*> dustbox_;
	bool (*compare_)(const unicode*,const unicode*,ulong);

public:

	KeywordMap( bool bCaseSensitive )
		: compare_( bCaseSensitive ? compare_s : compare_i )
	{
		// ハッシュ表初期化
		mem00( backet_, sizeof(backet_) );
	}

	~KeywordMap()
	{
		// 解放
		for( ulong i=0; i<dustbox_.size(); ++i )
			delete dustbox_[i];
	}

	void AddKeyword( const unicode* str, ulong len )
	{
		// データ登録
		Keyword* x = new Keyword(str,len);
		int      h = hash(str,len);

		if( backet_[h] == NULL )
		{
			// ハッシュテーブルが空の場合
			backet_[h] = x;
		}
		else
		{
			// チェイン末尾に繋ぐ場合
			Keyword *q=backet_[h],*p=backet_[h]->next;
			while( p!=NULL )
				q=p, p=p->next;
			q->next = x;
		}

		// データクリア用のリストにも入れておく
		dustbox_.Add(x);
	}

	ulong isKeyword( const unicode* str, ulong len )
	{
		// 登録されているキーワードと一致するか？
		for( Keyword* p=backet_[hash(str,len)]; p!=NULL; p=p->next )
			if( p->len==len && compare_( p->str, str, len ) )
				return 2;
		return 0;
	}

private:

	static int hash( const unicode* a, ulong al )
	{
		// 12bitに潰すめっちゃ雑なハッシュ関数
		// ルーチン分けるの面倒なので、大文字小文字は常に区別されない。(^^;
		int h=0,i=0;
		while( al-- )
		{
			h ^= ((*(a++)&0xdf)<<i);
			i = (i+5)&7;
		}
		return h&4095;
	}
};



//-------------------------------------------------------------------------
// 以上の道具立てでもって、テキストの解析を行うParser
//-------------------------------------------------------------------------
}

class editwing::doc::Parser
{
	KeywordMap kwd_;
	TagMap     tag_;

public:
	// 初期化１
	Parser(
		const unicode* cb, ulong cblen,
		const unicode* ce, ulong celen,
		const unicode* lb, ulong lblen,
		bool q1, bool q2, bool esc,
		bool casesensitive
	)
		: tag_( cb, cblen, ce, celen, lb, lblen, q1, q2, esc )
		, kwd_( casesensitive )
	{
	}

	// 初期化２：キーワード追加
	void AddKeyword( const unicode* str, ulong len )
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
		enum { T=0, W=4, A=8, S=12, O=0 };
		static const uchar letter_type[128] = {
			O,O,O,O,O,O,O,O,O,T,O,O,O,O,O,O,
			O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
			W,S,S,S,S,S,S,S,S,S,S,S,S,S,S,S,
			A,A,A,A,A,A,A,A,A,A,S,S,S,S,S,S,
			S,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,S,S,S,S,A,
			S,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,
			A,A,A,A,A,A,A,A,A,A,A,S,S,S,S,O,
		};

		// PosInToken算出用の距離エンコーダ( 5bitシフト済み )
		//  ( _d>7 ? 7<<5 : _d<<5 )
		#define tkenc(_d) ( (_d)>7 ? 0xe0 : (_d)<<5 )

		// コメント状態遷移追跡用オートマトン
		CommentDFA dfa[2] = {CommentDFA(false), CommentDFA(true)};
		int& cmtState  = dfa[line.isLineHeadCmt()].state;
		int commentbit = cmtState&1;

		// 作業領域
		int sym;
		ulong j, k, um, m;
		uchar t, f;

		// ループ～
		const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong           ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			j = i;

			// ASCII文字でない場合
			if( str[i] >= 0x007f )
			{
				f = (ALP | commentbit);
				if( str[i] == 0x3000 )//L'　' )
					while( str[++j] == 0x3000 )
						flg[j] = f;
				else
					while( str[++j] >= 0x80 && str[j]!=0x3000 )
						flg[j] = f;
				flg[i] = static_cast<uchar>(tkenc(j-i) | f);
			}
			// ASCII文字の場合??
			else
			{
				t = letter_type[str[i]];
				if( t==S && tag_.does_esc() )
					do
						if( j+1<ie && str[j]==L'\\' )
							j++;
					while( str[++j]<0x7f && S==letter_type[str[j]] );
				else
					while( str[++j]<0x7f && t==letter_type[str[j]] );

				f = (t | commentbit);

				switch( t )
				{
				// アルファベット＆数字
				case A:
					f |= kwd_.isKeyword( str+i, j-i );
					// fall...

				// タブ・制御文字
				case T:
					// fall...

				// 半角空白
				case W:
					for( k=i+1; k<j; ++k )
						flg[k] = f;
					flg[i] = (uchar)(tkenc(j-i)|f);
					break;

				// 記号
				case S:
					k = i;
					while( k < j )
					{
						// マッチしなかった部分
						um = tag_.SymbolLoop( str+k, j-k, m, sym );
						f = (0x20 | ALP | commentbit);
						while( um-- )
							flg[k++] = f;
						if( k >= j )
							break;

						// マッチした部分
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

		// transitフラグ更新
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
		ulong commentbit = dfa.state&1;

		// ループ～
		const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong       j,k,ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			// Tokenの終端を得る
			k = (flg[i]>>5);
			j = i + k;
			if( j >= ie )
				j = ie;
			else if( k==7 ) // || k==0 )
				while( (flg[j]>>5)==0 && j<ie )
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

DocImpl::DocImpl( Document& theDoc )
	: doc_    ( theDoc )
	, pEvHan_ ( 2 )
{
	text_.Add( new Line(L"",0) ); // 最初は一行だけ
	SetKeyword( NULL, 0 );        // キーワード無し
}

DocImpl::~DocImpl()
{
	// このファイルにデストラクタを入れておかないと、
	// delete parser_ が出来なくなる。^^;
}

void DocImpl::SetKeyword( const unicode* defbuf, ulong siz )
{
	// BOMがあったらスキップ
	if( siz!=0 && *defbuf==0xfeff )
		++defbuf, --siz;

	// 読み込み準備
	const unicode* str;
	ulong          len;
	UniReader r( defbuf, siz, &str, &len );
	bool          flags[] = {false,false,false,false};
	const unicode* tags[] = {NULL,NULL,NULL};
	ulong        taglen[] = {0,0,0};

	if( siz != 0 )
	{
		// １行目:フラグ
		//   case? q1? q2? esc?
		r.getLine();
		for( ulong i=0; i<len; ++i )
			flags[i] = (str[i]==L'1');

		// ２～４行目
		//   ブロコメ開始記号、ブロコメ終了記号、行コメ記号
		for( int j=0; j<3; ++j )
		{
			r.getLine();
			  tags[j] = str;
			taglen[j] = len;
		}
	}

	// パーサー作成
	aptr<Parser> np( new Parser(
		tags[0], taglen[0], tags[1], taglen[1], tags[2], taglen[2],
		flags[1], flags[2], flags[3], flags[0] ) );
	parser_ = np;

	// ５行目以降：キーワードリスト
	while( !r.isEmpty() )
	{
		r.getLine();
		if( len != 0 )
			parser_->AddKeyword( str, len );
	}

	// 全行解析し直し
	ReParse( 0, tln()-1 );

	// 変更通知
	Fire_KEYWORDCHANGE();
}

bool DocImpl::ReParse( ulong s, ulong e )
{
	ulong i;
	uchar cmt = text_[s].isLineHeadCmt();

	// まずは変更範囲を再解析
	for( i=s; i<=e; ++i )
		cmt = parser_->Parse( text_[i], cmt );

	// コメントアウト状態に変化がなかったらここでお終い。
	if( i==tln() || text_[i].isLineHeadCmt()==cmt )
		return false;

	// 例えば、/* が入力された場合などは、下の方の行まで
	// コメントアウト状態の変化を伝達する必要がある。
	do
		cmt = text_[i++].TransitCmt( cmt );
	while( i<tln() && text_[i].isLineHeadCmt()!=cmt );
	return true;
}

void DocImpl::SetCommentBit( const Line& x ) const
{
	parser_->SetCommentBit( const_cast<Line&>(x) );
}
