
#include "kilib/stdafx.h"
#include "RSearch.h"
#include "kilib/ktlaptr.h"
using namespace ki;



//=========================================================================
//@{
//	文字の種類
//@}
//=========================================================================

enum RegToken
{
	R_Char,  // 普通の文字
	R_Any,   // '.'
	R_Lcl,   // '['
	R_Rcl,   // ']'
	R_Ncl,   // '^'
	R_Range, // '-'
	R_Lbr,   // '('
	R_Rbr,   // ')'
	R_Bar,   // '|'
	R_Star,  // '*'
	R_Plus,  // '+'
	R_Quest, // '?'
	R_End    // '\0'
};



//=========================================================================
//@{
//	トークンに分解
//
//	行頭を表す^と行末を表す$については上位層で頑張る
//@}
//=========================================================================

class RegLexer
{
public:
	RegLexer( const wchar_t* pat, ulong len );
	RegToken GetToken();
	wchar_t  GetChar() { return chr_; }

private:
	const wchar_t* pat_;
	const wchar_t* end_;
	const wchar_t* sub_;
	wchar_t        chr_;
};



//=========================================================================
//@{
//	トークンに分解：実装
//@}
//=========================================================================

inline RegLexer::RegLexer( const wchar_t* pat, ulong len )
	: pat_( pat )
	, end_( pat+len )
	, sub_( L"" )
{
}

RegToken RegLexer::GetToken()
{
	const wchar_t*& x = (*sub_ ? sub_ : pat_);
	if( x == end_ ) return R_End;
	switch( *x++ )
	{
	case L'.': return R_Any;
	case L'[': return R_Lcl;
	case L']': return R_Rcl;
	case L'^': return R_Ncl;
	case L'-': return R_Range;
	case L'(': return R_Lbr;
	case L')': return R_Rbr;
	case L'|': return R_Bar;
	case L'*': return R_Star;
	case L'+': return R_Plus;
	case L'?': return R_Quest;
	case L'\\': if( x==end_ ) return R_End; switch( *x++ ) {
		case L't': chr_=L'\t';            return R_Char;
		case L'w': sub_=L"[0-9a-zA-Z_]";  return GetToken();
		case L'W': sub_=L"[^0-9a-zA-Z_]"; return GetToken();
		case L'd': sub_=L"[0-9]";         return GetToken();
		case L'D': sub_=L"[^0-9]";        return GetToken();
		case L's': sub_=L"[\t ]";         return GetToken();
		case L'S': sub_=L"[^\t ]";        return GetToken();
		} // fall through...
	default:
		chr_ = *(x-1);
		return R_Char;
	}
}



//=========================================================================
//@{
//	構文木のノードに振られる値の種類
//@}
//=========================================================================

enum RegType
{
	N_Char,     // 普通の文字 (ch)
	N_Class,    // [...] など (cls)
	N_Concat,   // 連接       (left, right)
	N_Or,       // |          (left, right)
	N_Closure,  // *          (left)
	N_Closure1, // +          (left)
	N_01,       // ?          (left)
	N_Empty     // 空         (--)
};

struct RegClass: public Object
{
	struct OneRange
	{
		wchar_t stt;
		wchar_t end;
	};
	OneRange      range;
	aptr<RegClass> next;
	RegClass( wchar_t s, wchar_t e, RegClass* n )
		{ aptr<RegClass> an(n); range.stt=s, range.end=e, next=an; }
};

struct RegNode: public Object
{
	RegNode()
	: type ( N_Char )
	, ch   ( L'\0' )
	, cls  ( NULL )
	, cmpcls (false)
	, left (NULL)
	, right (NULL)
	{
	}
	~RegNode()
	{
		if( left )  delete left;
		if( right ) delete right;
	}
	RegType           type; // このノードの種類
	wchar_t             ch; // 文字
	aptr<RegClass>     cls; // 文字集合
	bool            cmpcls; // ↑補集合かどうか
	RegNode          *left; // 左の子
	RegNode         *right; // 右の子
};



//=========================================================================
//@{
//	構文木作成
//@}
//=========================================================================

class RegParser: public Object
{
public:
	RegParser( const unicode* pat );
	~RegParser() { delete root_; }
	RegNode* root() { return root_; }
	bool err() { return err_; }
	bool isHeadType() const { return isHeadType_; }
	bool isTailType() const { return isTailType_; }

private:
	RegNode* make_empty_leaf();
	RegNode* make_char_leaf( wchar_t c );
	RegNode* make_node( RegType t, RegNode* lft, RegNode* rht );
	void eat_token();
	RegNode* expr();
	RegNode* term();
	RegNode* factor();
	RegNode* primary();
	RegNode* reclass();

private:
	bool    err_;
	bool    isHeadType_;
	bool    isTailType_;
	RegNode *root_;

	RegLexer lex_;
	RegToken nextToken_;
};



//=========================================================================
//@{
//	構文木作成：実装
//@}
//=========================================================================

namespace { static int tmp; }

inline RegParser::RegParser( const unicode* pat )
	: err_       ( false )
	, isHeadType_( *pat==L'^' )
	, isTailType_( (tmp=my_lstrlenW(pat), tmp && pat[tmp-1]==L'$') )
	, lex_(
		(isHeadType_ ? pat+1 : pat),
		(my_lstrlenW(pat) - (isHeadType_ ? 1 : 0)
		                  - (isTailType_ ? 1 : 0)) )
{
	eat_token();
	root_ = expr();
}

inline void RegParser::eat_token()
{
	nextToken_ = lex_.GetToken();
}

inline RegNode* RegParser::make_empty_leaf()
{
	RegNode* node = new RegNode;
	node->type = N_Empty;
	return node;
}

inline RegNode* RegParser::make_char_leaf( wchar_t c )
{
	RegNode* node = new RegNode;
	node->type = N_Char;
	node->ch   = c;
	return node;
}

RegNode* RegParser::make_node( RegType t, RegNode* lft, RegNode* rht )
{
	RegNode* node = new RegNode;
	node->type = t;
	node->left = lft;
	node->right= rht;
	return node;
}

RegNode* RegParser::reclass()
{
//	CLASS   ::= '^'? CHAR (CHAR | -CHAR)*

	bool neg = false;
	if( nextToken_ == R_Ncl )
		neg=true, eat_token();

	RegClass* cls = NULL;
	while( nextToken_ == R_Char )
	{
		wchar_t ch = lex_.GetChar();
		eat_token();
		if( nextToken_ == R_Range )
		{
			eat_token();
			if( nextToken_ != R_Char )
				err_ = true;
			else
			{
				wchar_t ch2 = lex_.GetChar();
				cls = new RegClass( Min(ch,ch2), Max(ch,ch2), cls );
				eat_token();
			}
		}
		else
		{
			cls = new RegClass( ch, ch, cls );
		}
	}

	RegNode* node = new RegNode;
	node->type   = N_Class;
	aptr<RegClass> ncls(cls);
	node->cls    = ncls;
	node->cmpcls = neg;
	return node;
}

RegNode* RegParser::primary()
{
//	PRIMARY ::= CHAR
//              '.'
//	            '[' CLASS ']'
//				'(' REGEXP ')'

	RegNode* node;
	switch( nextToken_ )
	{
	case R_Char:
		node = make_char_leaf( lex_.GetChar() );
		eat_token();
		break;
	case R_Any:{
		node         = new RegNode;
		node->type   = N_Class;
		aptr<RegClass> ncls(new RegClass( 0, 65535, NULL ));
		node->cls    = ncls;
		node->cmpcls = false;
		eat_token();
		}break;
	case R_Lcl:
		eat_token();
		node = reclass();
		if( nextToken_ == R_Rcl )
			eat_token();
		else
			err_ = true;
		break;
	case R_Lbr:
		eat_token();
		node = expr();
		if( nextToken_ == R_Rbr )
			eat_token();
		else
			err_ = true;
		break;
	default:
		node = make_empty_leaf();
		err_ = true;
		break;
	}
	return node;
}

RegNode* RegParser::factor()
{
//	FACTOR  ::= PRIMARY
//	            PRIMARY '*'
//			    PRIMARY '+'
//			    PRIMARY '?'

	RegNode* node = primary();
	switch( nextToken_ )
	{
	case R_Star: node=make_node(N_Closure,node,NULL); eat_token();break;
	case R_Plus: node=make_node(N_Closure1,node,NULL);eat_token();break;
	case R_Quest:node=make_node(N_01,node,NULL );     eat_token();break;
	default: break;
	}
	return node;
}

RegNode* RegParser::term()
{
//	TERM    ::= EMPTY
//	            FACTOR TERM

	if( nextToken_ == R_End )
		return make_empty_leaf();

	RegNode* node = factor();
	if( nextToken_==R_Lbr || nextToken_==R_Lcl
	 || nextToken_==R_Char|| nextToken_==R_Any )
		node = make_node( N_Concat, node, term() );
	return node;
}

RegNode* RegParser::expr()
{
//	REGEXP  ::= TERM
//	            TERM '|' REGEXP

	RegNode* node = term();
	if( nextToken_ == R_Bar )
	{
		eat_token();
		node = make_node( N_Or, node, expr() );
	}
	return node;
}



//=========================================================================
//@{
//	状態遷移
//@}
//=========================================================================

struct RegTrans : public Object
{
	enum {
		Epsilon,
		Class,
		Char
	}              type;
	aptr<RegClass>  cls; // この文字集合
	                     // orEpsilon が来たら
	bool         cmpcls;
	int              to; // 状態番号toの状態へ遷移

	aptr<RegTrans> next; // 連結リスト
/*
	template<class Cmp>
	bool match_i( wchar_t c, Cmp )
	{
		c = Cmp::map(c);
		RegClass* p = cls.get();
		while( p )
			if( Cmp::map(p->range.stt)<=c && c<=Cmp::map(p->range.end) )
				return true;
			else
				p = p->next.get();
		return false;
	}
*/
	bool match_c( wchar_t c )
	{
		for( RegClass* p=cls.get(); p; p=p->next.get() )
			if( p->range.stt<=c && c<=p->range.end )
				return true;
		return false;
	}

	bool match_i( wchar_t c )
	{
		c = IgnoreCase::map(c);
		for( RegClass* p=cls.get(); p; p=p->next.get() )
			if( IgnoreCase::map(p->range.stt)<=c
			 && c<=IgnoreCase::map(p->range.end) )
				return true;
		return false;
	}

	bool match( wchar_t c, bool caseS )
	{
		bool m = caseS ? match_c( c ) : match_i( c );
		return cmpcls ? !m : m;
	}
};



//=========================================================================
//@{
//	構文木->NFA変換
//@}
//=========================================================================

class RegNFA: public Object
{
public:
	RegNFA( const wchar_t* pat );
	~RegNFA();

	int match( const wchar_t* str, int len, bool caseS );
	bool isHeadType() const { return parser.isHeadType(); }
	bool isTailType() const { return parser.isTailType(); }

private:
	// マッチング処理
	int dfa_match( const wchar_t* str, int len, bool caseS );

	struct st_ele { int st, ps; };
	void push(storage<st_ele>& stack, int curSt, int pos);
	st_ele pop(storage<st_ele>& stack);

private:
	void add_transition( int from, wchar_t ch, int to );
	void add_transition( int from, aptr<RegClass> cls, bool cmp, int to );
	void add_e_transition( int from, int to );
	int gen_state();
	void gen_nfa( int entry, RegNode* t, int exit );

private:
	RegParser      parser;
	storage<RegTrans*> st;
	int      start, final;
};

RegNFA::RegNFA( const wchar_t* pat )
	: parser( pat )
{
	start = gen_state();
	final = gen_state();
	gen_nfa( start, parser.root(), final );
}

inline RegNFA::~RegNFA()
{
	for( ulong i=0,e=st.size(); i<e; ++i )
		delete st[i];
}

inline void RegNFA::add_transition
	( int from, aptr<RegClass> cls, bool cmp, int to )
{
	RegTrans* x = new RegTrans;
	aptr<RegTrans> nn( st[from] );
	x->next  = nn;
	x->to    = to;
	x->type  = RegTrans::Class;
	x->cls   = cls;
	x->cmpcls= cmp;
	st[from] = x;
}

inline void RegNFA::add_transition( int from, wchar_t ch, int to )
{
	aptr<RegClass> cls(new RegClass(ch,ch,NULL));
	add_transition( from, cls, false, to );
}

inline void RegNFA::add_e_transition( int from, int to )
{
	RegTrans* x = new RegTrans;
	aptr<RegTrans> nn( st[from] );
	x->next  = nn;
	x->to    = to;
	x->type  = RegTrans::Epsilon;
	st[from] = x;
}

inline int RegNFA::gen_state()
{
	st.Add( NULL );
	return st.size() - 1;
}

void RegNFA::gen_nfa( int entry, RegNode* t, int exit )
{
	switch( t->type )
	{
	case N_Char:
		//         ch
		//  entry ----> exit
		add_transition( entry, t->ch, exit );
		break;
	case N_Class:
		//         cls
		//  entry -----> exit
		add_transition( entry, t->cls, t->cmpcls, exit );
		break;
	case N_Concat: {
		//         left         right
		//  entry ------> step -------> exit
		int step = gen_state();
		gen_nfa( entry, t->left, step );
		gen_nfa( step, t->right, exit );
		} break;
	case N_Or:
		//          left
		//         ------>
		//  entry ------->--> exit
		//          right
		gen_nfa( entry, t->left, exit );
		gen_nfa( entry, t->right, exit );
		break;
	case N_Closure:
		//                       e
		//         e          <------        e
		//  entry ---> before ------> after ---> exit
		//    |                left                ^
		//    >------->------------------->------>-|
		//                      e
	case N_Closure1: {
		//                       e
		//         e          <------        e
		//  entry ---> before ------> after ---> exit
		//                     left
		int before = gen_state();
		int after = gen_state();
		add_e_transition( entry, before );
		add_e_transition( after, exit );
		add_e_transition( after, before );
		gen_nfa( before, t->left, after );
		if( t->type != N_Closure1 )
			add_e_transition( entry, exit );
		} break;
	case N_01:
		//           e
		//        ------>
		//  entry ------> exit
		//         left
		add_e_transition( entry, exit );
		gen_nfa( entry, t->left, exit );
		break;
	case N_Empty:
		//         e
		//  entry ---> exit
		add_e_transition( entry, exit );
		break;
	}
}



//=========================================================================
//@{
//	マッチング
//@}
//=========================================================================

void RegNFA::push(storage<st_ele>& stack, int curSt, int pos)
{
	// ε無限ループ防止策。同じ状態には戻らないように…
	for( int i=stack.size()-1; i>=0; --i )
		if( stack[i].ps != pos )
			break;
		else if( stack[i].st == curSt )
			return;

	st_ele nw = {curSt,pos};
	stack.Add( nw );
}

RegNFA::st_ele RegNFA::pop(storage<st_ele>& stack)
{
	st_ele se = stack[stack.size()-1];
	stack.ForceSize( stack.size()-1 );
	return se;
}

int RegNFA::match( const wchar_t* str, int len, bool caseS )
{
	if( parser.err() )
		return -1; // エラー状態なのでmatchとかできません
	//if( st.size() <= 31 )
	//	return dfa_match(str,len,caseS); // 状態数が少なければDFAを使う、かも

	int matchpos = -1;

	storage<st_ele> stack;
	push(stack, start, 0);
	while( stack.size() > 0 )
	{
		// スタックからpop
		st_ele se = pop(stack);
		int curSt = se.st;
		int   pos = se.ps;

		// マッチ成功してたら記録
		if( curSt == final ) // 1==終状態
			if( matchpos < pos )
				matchpos = pos;

		// さらに先の遷移を調べる
		if( matchpos < len )
		{
			for( RegTrans* tr=st[curSt]; tr!=NULL; tr=tr->next.get() )
			{
				if( tr->type == RegTrans::Epsilon )
					push(stack, tr->to, pos);
				else if( pos<len && tr->match( str[pos], caseS ) )
					push(stack, tr->to, pos+1);
			}
		}
	}

	return matchpos;
}

int RegNFA::dfa_match( const wchar_t* str, int len, bool caseS )
{
	int matchpos = -1;

	unsigned int StateSet = (1<<start);
	for(int pos=0; StateSet; ++pos)
	{
		// ε-closure
		for(uint DifSS=StateSet; DifSS;)
		{
			unsigned int NewSS = 0;
			for(int s=0; (1u<<s)<=DifSS; ++s)
				if( (1u<<s) & DifSS )
					for( RegTrans* tr=st[s]; tr!=NULL; tr=tr->next.get() )
						if( tr->type == RegTrans::Epsilon )
						 NewSS |= 1u << tr->to;
			DifSS = (NewSS|StateSet) ^ StateSet;
			StateSet |= NewSS;
		}

		// 受理状態を含んでるかどうか判定
		if( StateSet & (1<<final) )
			matchpos = pos;

		// 文字列の終わりに達した
		if( pos == len )
			break;

		// 遷移
		unsigned int NewSS = 0;
		for(int s=0; (1u<<s)<=StateSet; ++s)
			if( (1u<<s) & StateSet )
				for( RegTrans* tr=st[s]; tr!=NULL; tr=tr->next.get() )
					if( tr->type!=RegTrans::Epsilon && tr->match(str[pos], caseS) )
					 NewSS |= 1u << tr->to;
		StateSet = NewSS;
	}

	return matchpos;
}

//////////////////////////////////////////////////////////////////////

bool reg_match( const wchar_t* pat, const wchar_t* str, bool caseS )
{
	int len = my_lstrlenW(str);

	RegNFA re( pat );
	return len == re.match( str, len, caseS );
}



//=========================================================================
//@{
//	GreenPad用検索オブジェクト
//@}
//=========================================================================

RSearch::RSearch( const unicode* key, bool caseS, bool down )
	: re_    ( new RegNFA(key) )
	, caseS_ ( caseS )
	, down_  ( down )
{
}

RSearch::~RSearch()
{
	delete re_;
}

bool RSearch::Search(
	const unicode* str, ulong len, ulong stt, ulong* mbg, ulong* med )
{
	if( down_ && re_->isHeadType() && stt>0 )
		return false;

	const int d = (down_ ? 1 : -1);
	      int s = (!down_ && re_->isHeadType() ? 0 : stt);
	const int e = (down_ ? (re_->isHeadType() ? 1 : (long)len) : -1);

	for( ; s!=e; s+=d )
	{
		const int L = re_->match( str+s, len-s, caseS_ );
		if( L > 0 )
		{
			if( re_->isTailType() && L!=static_cast<int>(len-s) )
				continue;
			*mbg = static_cast<ulong>(s);
			*med = static_cast<ulong>(s+L);
			return true;
		}
	}

	return false;
}


