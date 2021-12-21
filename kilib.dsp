# Microsoft Developer Studio Project File - Name="kilib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=kilib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "kilib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "kilib.mak" CFG="kilib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "kilib - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "kilib - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "kilib - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE "kilib - Win32 NT31 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE "kilib - Win32 Win32s Ansi Release" (based on "Win32 (x86) Application")
!MESSAGE "kilib - Win32 Win4_Unicode release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "kilib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release"
# PROP Intermediate_Dir "OBJ/vc/rel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /Gr /Zp4 /W3 /Gi /Og /Oi /Os /Oy /Ob1 /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "SUPERTINY" /Yu"stdafx.h" /FD /GF /c
# SUBTRACT CPP /Gf
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 ole32.lib oleaut32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib delayimp.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib /out:"release/GreenPad.exe" /filealign:512 /OPT:REF /OPT:ICF,7 /delayload:IMM32.DLL
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "kilib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "OBJ/"
# PROP Intermediate_Dir "OBJ/vc/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /Zp1 /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "USEGLOBALIME" /D "UNICODE" /D "_UNICODE" /D "SUPERTINY" /D "UNICOWS" /D "DO_LOGGING" /D TARGET_VER=350 /FR /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 unicows.lib ole32.lib oleaut32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib delayimp.lib /nologo /subsystem:windows /profile /map:"release/GreenPad.map" /debug /machine:I386 /nodefaultlib /out:"release/GreenPad.exe" /filealign:512 /LARGEADDRESSAWARE

!ELSEIF  "$(CFG)" == "kilib - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kilib___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "kilib___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "release"
# PROP Intermediate_Dir "OBJ/vc/reluni"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /Zp4 /W3 /Gi /Og /Oi /Os /Oy /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "SUPERTINY" /D "USEGLOBALIME" /Yu"stdafx.h" /FD /GF /c
# SUBTRACT BASE CPP /Gf
# ADD CPP /nologo /Gr /Zp4 /W3 /Ox /Oa /Og /Oi /Os /Ob2 /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SUPERTINY" /D "USEGLOBALIME" /D "UNICODE" /D "_UNICODE" /D "UNICOWS" /D TARGET_VER=350 /Yu"stdafx.h" /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG" /d TARGT_VER=350
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib /out:"release/GreenPad.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib delayimp.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /nodefaultlib /out:"release/GreenPad.exe" /filealign:512 /OPT:REF /OPT:ICF,7 /LARGEADDRESSAWARE

!ELSEIF  "$(CFG)" == "kilib - Win32 NT31 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kilib___Win32_NT31_Unicode_Release"
# PROP BASE Intermediate_Dir "kilib___Win32_NT31_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "kilib___Win32_NT31_Unicode_Release"
# PROP Intermediate_Dir "kilib___Win32_NT31_Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /Zp4 /W3 /Gi /Og /Oi /Os /Oy /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SUPERTINY" /D "USEGLOBALIME" /D "UNICODE" /D "_UNICODE" /D TARGET_VER=350 /Yu"stdafx.h" /FD /GF /c
# ADD CPP /nologo /Gr /Zp4 /W3 /Ox /Oa /Og /Oi /Os /Ob2 /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SUPERTINY" /D "USEGLOBALIME" /D "UNICODE" /D "_UNICODE" /D "UNICOWS" /D TARGET_VER=310 /Yu"stdafx.h" /FD /GF /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d TARGET_VER=310
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib /out:"release/GreenPad.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib delayimp.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib /out:"release/GPadNT31.exe" /SUBSYSTEM:WINDOWS,3.10 /filealign:512 /OPT:REF /OPT:ICF,7 /LARGEADDRESSAWARE
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "kilib - Win32 Win32s Ansi Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kilib___Win32_Win32s_Ansi_Release"
# PROP BASE Intermediate_Dir "kilib___Win32_Win32s_Ansi_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "kilib___Win32_Win32s_Ansi_Release"
# PROP Intermediate_Dir "kilib___Win32_Win32s_Ansi_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /Zp4 /W3 /Gi /Og /Oi /Os /Oy /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "SUPERTINY" /D "USEGLOBALIME" /Yu"stdafx.h" /FD /GF /c
# SUBTRACT BASE CPP /Gf
# ADD CPP /nologo /Gr /Zp4 /W3 /Gi /Og /Oi /Os /Oy /Ob1 /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "SUPERTINY" /D "USEGLOBALIME" /D TARGET_VER=300 /D "WIN32S" /Yu"stdafx.h" /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib /out:"release/GreenPad.exe"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 ole32.lib oleaut32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib delayimp.lib /nologo /subsystem:windows /map /machine:I386 /nodefaultlib /out:"release/GPad32s.exe" /SUBSYSTEM:WINDOWS,3.10 /FIXED:NO /filealign:512 /OPT:REF /OPT:ICF,7
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "kilib - Win32 Win4_Unicode release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "kilib___Win32_Win4_Unicode_release"
# PROP BASE Intermediate_Dir "kilib___Win32_Win4_Unicode_release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "kilib___Win32_Win4_Unicode_release"
# PROP Intermediate_Dir "kilib___Win32_Win4_Unicode_release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /Zp4 /W3 /Ox /Og /Oi /Os /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SUPERTINY" /D "USEGLOBALIME" /D "UNICODE" /D "_UNICODE" /D "UNICOWS" /D TARGET_VER=350 /Yu"stdafx.h" /FD /GF /c
# ADD CPP /nologo /Gr /Zp4 /W3 /Ox /Og /Oi /Os /Ob2 /Gy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "SUPERTINY" /D "USEGLOBALIME" /D "UNICODE" /D "_UNICODE" /D "UNICOWS" /Yu"stdafx.h" /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG" /d TARGT_VER=350
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib /nologo /subsystem:windows /pdb:none /machine:I386 /nodefaultlib /out:"release/GPadNT35.exe"
# ADD LINK32 unicows.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib imm32.lib delayimp.lib /nologo /subsystem:windows /pdb:none /map /machine:I386 /nodefaultlib /out:"release/GPadNew.exe" /filealign:512 /OPT:REF /OPT:ICF,7 /delayload:IMM32.DLL

!ENDIF 

# Begin Target

# Name "kilib - Win32 Release"
# Name "kilib - Win32 Debug"
# Name "kilib - Win32 Unicode Release"
# Name "kilib - Win32 NT31 Unicode Release"
# Name "kilib - Win32 Win32s Ansi Release"
# Name "kilib - Win32 Win4_Unicode release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ConfigManager.cpp

!IF  "$(CFG)" == "kilib - Win32 Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Debug"

!ELSEIF  "$(CFG)" == "kilib - Win32 Unicode Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "kilib - Win32 NT31 Unicode Release"

# ADD CPP /Ox /Oa /Og

!ELSEIF  "$(CFG)" == "kilib - Win32 Win32s Ansi Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Win4_Unicode release"

# ADD BASE CPP /Oa
# ADD CPP /Oa

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\GpMain.cpp

!IF  "$(CFG)" == "kilib - Win32 Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Debug"

!ELSEIF  "$(CFG)" == "kilib - Win32 Unicode Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "kilib - Win32 NT31 Unicode Release"

# ADD CPP /Ox /Oa /Og

!ELSEIF  "$(CFG)" == "kilib - Win32 Win32s Ansi Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Win4_Unicode release"

# ADD BASE CPP /Oa
# ADD CPP /Oa

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\OpenSaveDlg.cpp

!IF  "$(CFG)" == "kilib - Win32 Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Debug"

!ELSEIF  "$(CFG)" == "kilib - Win32 Unicode Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "kilib - Win32 NT31 Unicode Release"

# ADD CPP /Ox /Oa /Og

!ELSEIF  "$(CFG)" == "kilib - Win32 Win32s Ansi Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Win4_Unicode release"

# ADD BASE CPP /Oa
# ADD CPP /Oa

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\RSearch.cpp

!IF  "$(CFG)" == "kilib - Win32 Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Debug"

!ELSEIF  "$(CFG)" == "kilib - Win32 Unicode Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "kilib - Win32 NT31 Unicode Release"

# ADD CPP /Ox /Oa /Og

!ELSEIF  "$(CFG)" == "kilib - Win32 Win32s Ansi Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Win4_Unicode release"

# ADD BASE CPP /Oa
# ADD CPP /Oa

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Search.cpp

!IF  "$(CFG)" == "kilib - Win32 Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Debug"

!ELSEIF  "$(CFG)" == "kilib - Win32 Unicode Release"

# ADD CPP /Oa

!ELSEIF  "$(CFG)" == "kilib - Win32 NT31 Unicode Release"

# ADD CPP /Ox /Oa /Og

!ELSEIF  "$(CFG)" == "kilib - Win32 Win32s Ansi Release"

!ELSEIF  "$(CFG)" == "kilib - Win32 Win4_Unicode release"

# ADD BASE CPP /Oa
# ADD CPP /Oa

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ConfigManager.h
# End Source File
# Begin Source File

SOURCE=.\GpMain.h
# End Source File
# Begin Source File

SOURCE=.\NSearch.h
# End Source File
# Begin Source File

SOURCE=.\OpenSaveDlg.h
# End Source File
# Begin Source File

SOURCE=.\RSearch.h
# End Source File
# Begin Source File

SOURCE=.\Search.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\rsrc\exefile.ico
# End Source File
# Begin Source File

SOURCE=.\rsrc\gp_rsrc.rc
# End Source File
# Begin Source File

SOURCE=.\rsrc\manifest.xml
# End Source File
# Begin Source File

SOURCE=.\rsrc\resource.h
# End Source File
# End Group
# Begin Group "kilib"

# PROP Default_Filter ""
# Begin Group "Source"

# PROP Default_Filter "cpp"
# Begin Source File

SOURCE=.\kilib\app.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\cmdarg.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\ctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\file.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\find.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\log.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\memory.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\path.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\registry.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\kilib\string.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\textfile.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\thread.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\window.cpp
# End Source File
# Begin Source File

SOURCE=.\kilib\winutil.cpp
# End Source File
# End Group
# Begin Group "Header"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\kilib\app.h
# End Source File
# Begin Source File

SOURCE=.\kilib\cmdarg.h
# End Source File
# Begin Source File

SOURCE=.\kilib\ctrl.h
# End Source File
# Begin Source File

SOURCE=.\kilib\file.h
# End Source File
# Begin Source File

SOURCE=.\kilib\find.h
# End Source File
# Begin Source File

SOURCE=.\kilib\log.h
# End Source File
# Begin Source File

SOURCE=.\kilib\memory.h
# End Source File
# Begin Source File

SOURCE=.\kilib\path.h
# End Source File
# Begin Source File

SOURCE=.\kilib\registry.h
# End Source File
# Begin Source File

SOURCE=.\kilib\string.h
# End Source File
# Begin Source File

SOURCE=.\kilib\textfile.h
# End Source File
# Begin Source File

SOURCE=.\kilib\thread.h
# End Source File
# Begin Source File

SOURCE=.\kilib\types.h
# End Source File
# Begin Source File

SOURCE=.\kilib\window.h
# End Source File
# Begin Source File

SOURCE=.\kilib\winutil.h
# End Source File
# End Group
# Begin Group "KTL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\kilib\ktlaptr.h
# End Source File
# Begin Source File

SOURCE=.\kilib\ktlarray.h
# End Source File
# Begin Source File

SOURCE=.\kilib\ktlgap.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\kilib\kilib.h
# End Source File
# Begin Source File

SOURCE=.\kilib\stdafx.h
# End Source File
# End Group
# Begin Group "editwing"

# PROP Default_Filter ""
# Begin Group "public"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\editwing\ewCommon.h
# End Source File
# Begin Source File

SOURCE=.\editwing\ewCtrl1.h
# End Source File
# Begin Source File

SOURCE=.\editwing\ewDoc.h
# End Source File
# Begin Source File

SOURCE=.\editwing\ewView.h
# End Source File
# End Group
# Begin Group "private"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\editwing\ip_ctrl1.cpp
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_cursor.cpp
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_doc.h
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_draw.cpp
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_parse.cpp
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_scroll.cpp
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_text.cpp
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_view.h
# End Source File
# Begin Source File

SOURCE=.\editwing\ip_wrap.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\editwing\editwing.h
# End Source File
# End Group
# End Target
# End Project
