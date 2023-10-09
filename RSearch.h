#ifndef AFX_RSEARCH_H__5A9346D4_3152_4923_8EFC_38264A456364__INCLUDED_
#define AFX_RSEARCH_H__5A9346D4_3152_4923_8EFC_38264A456364__INCLUDED_
#include "NSearch.h"


//=========================================================================
//@{ @pkg Gp.Search //@}
//@{
//	超簡易正規表現マッチング関数。
//
//	patとstr全体がマッチするならtrue、ダメならfalseを返す
//@}
//=========================================================================

bool reg_match( const wchar_t* pat, const wchar_t* str, bool caseS );


class RegNFA;
//=========================================================================
//@{
// Searhcableとしての実装
//@}
//=========================================================================

class RSearch A_FINAL: public Searchable
{
public:
	explicit RSearch( const unicode* key, bool caseS, bool down );
	~RSearch();

private:
	bool Search( const unicode* str, ulong len, ulong stt,
		ulong* mbg, ulong* med ) override;

private:
	RegNFA *re_;
	bool caseS_;
	bool down_;

	NOCOPY(RSearch);
};





#endif
