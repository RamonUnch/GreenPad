#include "stdafx.h"

#if defined(TARGET_VER) && TARGET_VER <= 303
	// DragQueryFileW is not present NT 3.10.340.
	// Use ANSI version instead that will have to be
	// renamed from DragQueryFileA -> DragQueryFile
	UINT myDragQueryFileW(HDROP hd, UINT i, LPWSTR wpath, UINT l)
	{// Use ansi API and convert string...
		if( i == 0xffffffff )
			return DragQueryFileA( hd, 0xffffffff, NULL, 0 );
		char buf[MAX_PATH];
		UINT ret = DragQueryFileA(hd, i, buf, l);
		if( ret && wpath && l)
			::MultiByteToWideChar( CP_ACP, 0, buf,MAX_PATH, wpath,l );

		return ret;
	}
#endif
