#include "stdafx.h"
#include "ip_doc.h"
using namespace editwing;
using namespace editwing::doc;



//=========================================================================
//---- ip_parse.cpp �L�[���[�h���
//
//		�L�[���[�h��`�t�@�C���ɏ]���āA�ێ����镶�����
//		�K�؂ɐ؂蕪�����Ƃ������B
//
//---- ip_text.cpp   �����񑀍�E��
//---- ip_wrap.cpp   �܂�Ԃ�
//---- ip_scroll.cpp �X�N���[��
//---- ip_draw.cpp   �`��E��
//---- ip_cursor.cpp �J�[�\���R���g���[��
//=========================================================================



//=========================================================================
//
// ��͌��ʃf�[�^�d�l
// ���ꂾ���F�X�Ƒ��Ȏ�i����������Ŗ{����
// �����Ȃ��Ă���̂��ǂ����͕s���c(^^;
//
// -----------------------------------------------
//
// Line::isLineHeadCommented_
//    0: �s�����u���b�N�R�����g�̓����ł͂Ȃ�
//    1: �s�����u���b�N�R�����g�̓���
//
// -----------------------------------------------
//
// Line::commentTransition_
//   00: �s���͏�ɃR�����g�̊O
//   01: �s���ƍs���̓R�����g��Ԃ��t�]
//   10: �s���ƍs���̓R�����g��Ԃ�����
//   11: �s���͏�ɃR�����g�̒�
//
// -----------------------------------------------
//
// �ȏ��̃t���O�����ɁA�O�̍s�̏�񂩂獡�̍s�̏���
//   this.head = (prev.trans >> prev.head)&1;
// �ŏ����v�Z���Ă������Ƃ��o����B
// ���̌v�Z�̍ۂɓ����o�b�t�@�̏�Ԃ܂ŏ���������̂�
// �R�X�g���ł�������̂ŁA���Ɏ����t���O�����Ȃ���
// �`�搡�O�ɓK�X��������B
//
// -----------------------------------------------
//
// Line::commentBitReady_
//   �R�����g�r�b�g�������ς݂��ǂ���
//
// -----------------------------------------------
//
// Line::str_[]
//   UCS-2�x�^�ŁA������f�[�^�����̂܂܊i�[�����B
//   �������A�p�[�T�̍������̂��߂ɍŏI�����̌���
//   0x007f���t�������B
//
// -----------------------------------------------
//
// Line::flg_
//   �ꕶ�����ɁA���̂悤��8bit�̃t���O�����蓖�Ă�
//   | aaabbbcd |
// 
// -----------------------------------------------
//
// aaa == "PosInToken"
//     0: �g�[�N���̓r��
//   1-6: �g�[�N���̓��B���̓���1-6������B
//     7: �g�[�N���̓��B���̓���7�����ȏ��B
//
// -----------------------------------------------
//
// bbb == "TokenType"
//     0: TAB: �^�u����
//     1: WSP: �z���C�g�X�y�[�X
//     2: TXT: ���ʂ̕���
//     3:  CE: �R�����g�J�n�^�O
//     4:  CB: �R�����g�I���^�O
//     5:  LB: �s�R�����g�J�n�^�O
//     6:  Q1: '' ���p��1
//     7:  Q2: "" ���p��2
//
// -----------------------------------------------
//
//  c  == "isKeyword?"
//     0: �L�[���[�h�ł͂Ȃ�
//     1: �L�[���[�h
//
// -----------------------------------------------
//
//  d  == "inComment?"
//     0: �R�����g�̒��ł͂Ȃ�
//     1: �R�����g�̒�
//
// -----------------------------------------------



namespace {
//-------------------------------------------------------------------------
// �R�����g�̒��Ȃ̂��O�Ȃ̂����𔻒肷�邽�߂̃I�[�g�}�g��
//
// /* ���o���炻�̌��̓R�����g�� */ ���o���炻�̌��̓m�[�}���]�[��
// �c�Ƃ����P���ȋK���ł͏�肭�s���Ȃ��B�Ⴆ�� str"/*"str �Ȃ�Ă��̂�
// �o�������ꍇ�ɍ����Ă��܂��B�����ŁA
//   �E���ʂ̃e�L�X�g
//   �E�u���b�N�R�����g�̒�
//   �E�s�R�����g�̒�
//   �E��d���p���̒�
//   �E��d���p���̒�
// �̂T��ނ̏�Ԃɕ����āA���ꂼ��̏ꍇ�ɂ��āA�ǂ̋L�����o����
// ���ɂǂ̏�ԂɈڂ�̂��c����������K�v������B���̏�ԕω��̋K����
// 5x5�̂Q�����z��ŗ^���ĊǗ�����B
//-------------------------------------------------------------------------

enum CommentDFASymbol{ sCB, sCE, sLB, sQ1, sQ2, sXXX };
struct CommentDFA
{
	// <���>
	// �ŉ���bit���A���݃R�����g�����ǂ����̃t���O�ɂȂ�܂��B
	// �u���b�N�R�����g�����ǂ����� (state>>1)&(state) �ŁB
	//   000: normal text        011: in BlockComment
	//   001: in LineComment     100: in Quote2
	//   010: in Quote1
	//
	// <�V���{��>
	// C++�Ō����Ɖ��̒ʂ�
	// �l��TokenType�t���O�ƃV���N������悤�ɂȂ��Ă܂��B
	//   000: CE */              011: Q1 '
	//   001: CB /*              100: Q2 "
	//   010: LB //

	// ������Ԃ��w��B�R�����g�����R�����g�O��
	CommentDFA( bool inComment )
		: state( inComment ? 3 : 0 ) {}

	// ���͕�����^���ď�ԑJ��
	void transit( int sym ) 
		{ state = tr_table[state][sym]; }

	// ���݂̏��
	int state;

	// ��ԑJ�ڃe�[�u��
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
// �P���ȁA�L�[���[�h�i�[�\���́B
// ChainHash�̗v�f�ɂ��邽��next�|�C���^�����Ă���܂��B
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
// �T�|�[�g�֐��BUnicode�e�L�X�g���m�̔�r
//-------------------------------------------------------------------------

static bool compare_s(const unicode* a,const unicode* b,ulong l)
{
	// �啶�������������
	while( l-- )
		if( *a++ != *b++ )
			return false;
	return true;
}

static bool compare_i(const unicode* a,const unicode* b,ulong l)
{
	// �啶������������ʂ��Ȃ��i�G�j
	while( l-- )
		if( ((*a++) ^ (*b++)) & 0xdf )
			return false;
	return true;
}



//-------------------------------------------------------------------------
// �^����ꂽ�L�������񂩂�A�R�����g�J�n���̈Ӗ��̂���g�[�N����
// �؂�o���Ă��邽�߂̍\���B
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
		// '/' �Ŏn�܂�L���͎g���Ă��邩�c�H
		// �݂����ȁA�P�����ڂ݂̂̃`�F�b�N�Ɏg���\���쐬
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
		// �L�[���[�h���
		delete tag_[0];
		delete tag_[1];
		delete tag_[2];
	}

	bool does_esc()
	{
		// \ �ɂ��G�X�P�[�v�����邩�ǂ���
		return esc_;
	}

	ulong SymbolLoop(
		const unicode* str, ulong len, ulong& mlen, int& sym )
	{
		// �L�Ӗ��ȋL���Ƀ}�b�`����܂Ń��[�v
		// �Ԓl�ɁA�}�b�`����܂łɔ�΂����������A
		// mlen,sym�ɁA�}�b�`�����L���̏���Ԃ�

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
				if( str[ans] == L'\'' ) // ��d���p��
				{
					if( q1_ )
					{
						sym  = sQ1;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\"' ) // ��d���p��
				{
					if( q2_ )
					{
						sym  = sQ2;
						goto symbolfound;
					}
				}
				else if( str[ans] == L'\\' ) // \ �̌�̕�����Skip
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
// �^����ꂽ�����񂪃L�[���[�h���ǂ����������肷�邽�߂̃n�b�V���\
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
		// �n�b�V���\������
		mem00( backet_, sizeof(backet_) );
	}

	~KeywordMap()
	{
		// ���
		for( ulong i=0; i<dustbox_.size(); ++i )
			delete dustbox_[i];
	}

	void AddKeyword( const unicode* str, ulong len )
	{
		// �f�[�^�o�^
		Keyword* x = new Keyword(str,len);
		int      h = hash(str,len);

		if( backet_[h] == NULL )
		{
			// �n�b�V���e�[�u������̏ꍇ
			backet_[h] = x;
		}
		else
		{
			// �`�F�C�������Ɍq���ꍇ
			Keyword *q=backet_[h],*p=backet_[h]->next;
			while( p!=NULL )
				q=p, p=p->next;
			q->next = x;
		}

		// �f�[�^�N���A�p�̃��X�g�ɂ�����Ă���
		dustbox_.Add(x);
	}

	ulong isKeyword( const unicode* str, ulong len )
	{
		// �o�^����Ă���L�[���[�h�ƈ�v���邩�H
		for( Keyword* p=backet_[hash(str,len)]; p!=NULL; p=p->next )
			if( p->len==len && compare_( p->str, str, len ) )
				return 2;
		return 0;
	}

private:

	static int hash( const unicode* a, ulong al )
	{
		// 12bit�ɒׂ��߂�����G�ȃn�b�V���֐�
		// ���[�`��������̖ʓ|�Ȃ̂ŁA�啶���������͏�ɋ�ʂ���Ȃ��B(^^;
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
// �ȏ�̓���Ăł����āA�e�L�X�g�̉�͂��s��Parser
//-------------------------------------------------------------------------
}

class editwing::doc::Parser
{
	KeywordMap kwd_;
	TagMap     tag_;

public:
	// �������P
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

	// �������Q�F�L�[���[�h�ǉ�
	void AddKeyword( const unicode* str, ulong len )
	{
		kwd_.AddKeyword( str, len );
	}

	// �s�f�[�^���
	uchar Parse( Line& line, uchar cmst )
	{
		line.TransitCmt( cmst );

		// ASCII�U�蕪���e�[�u���B
		// �V�t�g������TokenType�ɗ��p�o����悤�ɂ��邽�߁A
		// �l���S��тɂȂ��Ă܂�
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

		// PosInToken�Z�o�p�̋����G���R�[�_( 5bit�V�t�g�ς� )
		//  ( _d>7 ? 7<<5 : _d<<5 )
		#define tkenc(_d) ( (_d)>7 ? 0xe0 : (_d)<<5 )

		// �R�����g��ԑJ�ڒǐ՗p�I�[�g�}�g��
		CommentDFA dfa[2] = {CommentDFA(false), CommentDFA(true)};
		int& cmtState  = dfa[line.isLineHeadCmt()].state;
		int commentbit = cmtState&1;

		// ��Ɨ̈�
		int sym;
		ulong j, k, um, m;
		uchar t, f;

		// ���[�v�`
		const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong           ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			j = i;

			// ASCII�����łȂ��ꍇ
			if( str[i] >= 0x007f )
			{
				f = (ALP | commentbit);
				if( str[i] == 0x3000 )//L'�@' )
					while( str[++j] == 0x3000 )
						flg[j] = f;
				else
					while( str[++j] >= 0x80 && str[j]!=0x3000 )
						flg[j] = f;
				flg[i] = static_cast<uchar>(tkenc(j-i) | f);
			}
			// ASCII�����̏ꍇ??
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
				// �A���t�@�x�b�g������
				case A:
					f |= kwd_.isKeyword( str+i, j-i );
					// fall...

				// �^�u�E���䕶��
				case T:
					// fall...

				// ���p��
				case W:
					for( k=i+1; k<j; ++k )
						flg[k] = f;
					flg[i] = (uchar)(tkenc(j-i)|f);
					break;

				// �L��
				case S:
					k = i;
					while( k < j )
					{
						// �}�b�`���Ȃ���������
						um = tag_.SymbolLoop( str+k, j-k, m, sym );
						f = (0x20 | ALP | commentbit);
						while( um-- )
							flg[k++] = f;
						if( k >= j )
							break;

						// �}�b�`��������
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

		// transit�t���O�X�V
		line.SetTransitFlag(
			(dfa[1].state & (dfa[1].state<<1)) |
			((dfa[0].state>>1) & dfa[0].state)
		);
		line.CommentBitUpdated();
		return line.TransitCmt( cmst );
	}

	// �R�����g�r�b�g�𐳂�������
	void SetCommentBit( Line& line )
	{
		CommentDFA dfa( line.isLineHeadCmt()==1 );
		ulong commentbit = dfa.state&1;

		// ���[�v�`
		const unicode* str = line.str();
		uchar*         flg = line.flg();
		ulong       j,k,ie = line.size();
		for( ulong i=0; i<ie; i=j )
		{
			// Token�̏I�[�𓾂�
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
// ��`�t�@�C���ǂ݂Ƃ菈���Ƃ�
//-------------------------------------------------------------------------

DocImpl::DocImpl( Document& theDoc )
	: doc_    ( theDoc )
	, pEvHan_ ( 2 )
{
	text_.Add( new Line(L"",0) ); // �ŏ��͈�s����
	SetKeyword( NULL, 0 );        // �L�[���[�h����
}

DocImpl::~DocImpl()
{
	// ���̃t�@�C���Ƀf�X�g���N�^�����Ă����Ȃ��ƁA
	// delete parser_ ���o���Ȃ��Ȃ�B^^;
}

void DocImpl::SetKeyword( const unicode* defbuf, ulong siz )
{
	// BOM����������X�L�b�v
	if( siz!=0 && *defbuf==0xfeff )
		++defbuf, --siz;

	// �ǂݍ��ݏ���
	const unicode* str;
	ulong          len;
	UniReader r( defbuf, siz, &str, &len );
	bool          flags[] = {false,false,false,false};
	const unicode* tags[] = {NULL,NULL,NULL};
	ulong        taglen[] = {0,0,0};

	if( siz != 0 )
	{
		// �P�s��:�t���O
		//   case? q1? q2? esc?
		r.getLine();
		for( ulong i=0; i<len; ++i )
			flags[i] = (str[i]==L'1');

		// �Q�`�S�s��
		//   �u���R���J�n�L���A�u���R���I���L���A�s�R���L��
		for( int j=0; j<3; ++j )
		{
			r.getLine();
			  tags[j] = str;
			taglen[j] = len;
		}
	}

	// �p�[�T�[�쐬
	aptr<Parser> np( new Parser(
		tags[0], taglen[0], tags[1], taglen[1], tags[2], taglen[2],
		flags[1], flags[2], flags[3], flags[0] ) );
	parser_ = np;

	// �T�s�ڈȍ~�F�L�[���[�h���X�g
	while( !r.isEmpty() )
	{
		r.getLine();
		if( len != 0 )
			parser_->AddKeyword( str, len );
	}

	// �S�s��͂�����
	ReParse( 0, tln()-1 );

	// �ύX�ʒm
	Fire_KEYWORDCHANGE();
}

bool DocImpl::ReParse( ulong s, ulong e )
{
	ulong i;
	uchar cmt = text_[s].isLineHeadCmt();

	// �܂��͕ύX�͈͂��ĉ��
	for( i=s; i<=e; ++i )
		cmt = parser_->Parse( text_[i], cmt );

	// �R�����g�A�E�g��Ԃɕω����Ȃ������炱���ł��I���B
	if( i==tln() || text_[i].isLineHeadCmt()==cmt )
		return false;

	// �Ⴆ�΁A/* �����͂��ꂽ�ꍇ�Ȃǂ́A���̕��̍s�܂�
	// �R�����g�A�E�g��Ԃ̕ω���`�B����K�v������B
	do
		cmt = text_[i++].TransitCmt( cmt );
	while( i<tln() && text_[i].isLineHeadCmt()!=cmt );
	return true;
}

void DocImpl::SetCommentBit( const Line& x ) const
{
	parser_->SetCommentBit( const_cast<Line&>(x) );
}
