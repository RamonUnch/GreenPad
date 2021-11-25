#ifndef AFX_RSEARCH_H__5A9346D4_3152_4923_8EFC_38264A456364__INCLUDED_
#define AFX_RSEARCH_H__5A9346D4_3152_4923_8EFC_38264A456364__INCLUDED_
#include "kilib/ktlaptr.h"
#include "NSearch.h"


//=========================================================================
//@{ @pkg Gp.Search //@}
//@{
//	���ȈՐ��K�\���}�b�`���O�֐��B
//
//	pat��str�S�̂��}�b�`����Ȃ�true�A�_���Ȃ�false��Ԃ�
//@}
//=========================================================================

bool reg_match( const wchar_t* pat, const wchar_t* str, bool caseS );


class RegNFA;
//=========================================================================
//@{
// Searhcable�Ƃ��Ă̎���
//@}
//=========================================================================

class RSearch : public Searchable
{
public:
	RSearch( const unicode* key, bool caseS, bool down );

private:
	virtual bool Search( const unicode* str, ulong len, ulong stt,
		ulong* mbg, ulong* med );

private:
	ki::dptr<RegNFA> re_;
	bool caseS_;
	bool down_;
};





#endif
