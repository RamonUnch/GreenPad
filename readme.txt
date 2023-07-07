=<> GreenPad ver 1.19
=<> RamonUnch builds 2023/06/23

<<What's This?>>

  GreenPad is a tiny text editor for Windows.

  It aims to be a handy Notepad replacement with minimal but
  complete features, not to be a rich, bloated monsterous
  editor. GreenPad supports:
    * Unicode 14.0
    * Proportional Fonts
    * Syntax Highlighting
    * Searching with Regular Expressions
  while keeping the size of .exe very small (around 130KB!).

  Freeware, distributed under the NYSL licence.
  Original source code is available at: http://www.kmonos.net/lib/gp.en.html
  New source here: https://github.com/RamonUnch/GreenPad
  Supported OSes: Win32s/NT3x/NT4/9x/ME/2000/XP/Vista/7/8.x/10/11, ReactOS.

<<System Requirements>>
 < HARDWARE >
   i386 cpu or compatible and at least a couple of MB of free RAM.
   Tested with Win3.11fw + Win32s-1.30c with 3MB of ram. Performances
   are not great on an i386 even clocked at 33MHz and an i486 or faster
   is recomended. GreenPad will not benefit from a x87 FPU. However
   chardet.dll will.

 < OS >
   You can run Green pad on Win32s from beta build 61. If you want the
   statusbar on Win32s 1.25 you can try to copy COMCTL32.DLL version
   3.50.807.1 from Win32s 1.25.127 Also GreenPad Should run on
   Windows NT3.10.340 and later. GreenPad was also tested with early
   Chicago builds. You could however consider upgrading to Win95...

   GreenPad was also tested on the WIN-OS/2 subsystem for OS/2
   you will need to first install Win32s (1.25a recomended).
   Also the ansi version of GreenPad appears to work with Odin.

   GreenPad also supports ReactOS which can be compared to 2000/XP
   in term of application support.

<<To Do List>>
  * Build for all possible CPUs: MIPS, PowerPC, Alpha, ARM, ARM64.
    roytam1 already made a MIPS compatible build.
  * Handle WS_EX_LAYOUTRTL for left to right scripts such as Hebrew.
  * Handle more complete regular expression and especially capture groups
    for proper find replace.
  * Optimize Find/Replace so that it does not take for ever to replace
    a word in a big file.
  * Handle binary files properly: We will need to flag each line with the
    carriage return type CRLF, LF or CR.
  * Handle per-monitor dpi awareness. This will require to reload the
    Painter each time we receive the WM_DPICHANGED message.
  * Improve syntactic coloration (highlight matching braces/parenthesis)
  * Handle Dark mode in the non-client area for Windows 10 1607+
  * Allow a Square selection mode for ASCII-Art or column handling.
  * Improve printer configuration.
  * Fix more bugs, handle low memory situations better.
  * Optimize memory usage so that larger files can be loaded.

<<What's New in 1.19 (by RamonUnch, 2023/07/07)>>
 < NEW >
  * Added the ability to go to matching brace with Ctrl+B.

 < FIXED >
  * Fixed Unicode reader for UTF-1, UTF-9, FSS-UTF and writer for SCSU.
  * Warn when trying to load a huge file (larger than 2GB),
    and propose to retry loading only the first 64KB.
  * Sometime a setting would not be written to ini file properly.
  * Find dialog settings will be saved even on cancel.
  * Fixed a potential crash in rightOf() function. #121
  * Minor refactoring: ensure some variables are always initialized,
    remove redundent checks, be more carfeful with some lifetimes.
    use const and explicit when possible...

<<What's New in 1.18 (by RamonUnch, 2023/06/23)>>
 < NEW >
  * Display codepage number in the status bar.
  * Allow to directly type codepage number in Save/Open/ReOpen dialogs
  * Added Help->About dialog that displays GreenPad version and build
    details as well as the Currently running Windows version.
  * Optimize memmove for gcc build (significantly faster).
  * Minor refactoring, reduces mem usage a few percents.
  * Increase Win32s scrolling range from 8 million up to 24 million lines
  * Various optimizations for file reading/writing code.

 < FIXED >
  * Crash when opening the Save dialog if the current codepage is outside
    the default cs list.
  * Avoid false UTF-16/32 detection for some files.
  * The Tab key can now be used to navigate through the Open/Save dialog.
  * Fixed Crash when reading invalid Iso2022 or UtfOFSS files.
  * Directly use current local codepage when detecting a pure ASCII file.
  * Minor code refactoring, reducing exe size.
  * Use more stack buffers when possible instead of new/delete.
  * Avoid excessive memory allocation when saving a file, also avoid
    useless memmove between two intermediate buffers and improve perf.

<<What's New in 1.17 (by RamonUnch, 2023/03/11)>>
 < NEW >
  * OLE2 style drag and drop support was added in most builds
    To use it you will need to use the right click (easier to code)
    Also Win32s 1.25+ or NT3.50+ are needed.
  * The Help->About menu and Dialog were added to GreenPad.

 < FIXED >
  * Loading is now significantly faster for large files.
  * Out of bound read in the internal memory manager.
  * Various refactoring and bugfixes.
  * Now the `CS_OWNDC` class style is no longer used so GreenPad is
    Compatible with ReactOS and does not mess up with DC cache.
  * Now the GPad32sl.exe version is compatible with the early chicago
    builds and with Win32s beta 61.
  * Now the GPadNT3b.exe version is compatible with the NT3.10.340 beta.
  * Now the main GreenPad Window can be resized on Win32s 1.15a
  * Now GPad32s.exe run on Win32s 1.1 (1993), fully functional.
  * Fixed Visual glitches with Windows 95 beta (chicago) thanks to roytam1
  * Elevation dialog for write permission will not pop up on network drives
    nor CD-ROM drives (Windows 2000 + only)

<<What's New in 1.16 (by RamonUnch, 2023/01/01)>>
 < NEW >
  * Wrapping now occurs at word boundaries this helps readability.
    set ws=1/0 in the layout file to enable/disable. Character that can
    split include all control characters and all characters beyond U+2500
  * Now control characters will be displayed as colored ASCII characters
    if the display for ideographic spaces is enabled (U+3000).
    This is helpful when looking up at binary files.
  * Win32s build now will display full Unicode if it runs on Win9x/NT.
  * Win32s build now support different key-maps on Win9x/NT
  * Add support for UAC, now if a file cannot be opened by the current user
    GreenPad will prompt the user with the UAC pannel.
    Also the Open elevated (Ctrl+L) option was added to the file menu.
    Note: Elevating a process requires a new instance.

 < FIXED >
  * Fixed crash on Ctrl+N on Win9x using unicows build.
  * Fixed 64bit build.
  * more 64bit index are now used on 64 bit builds.
  * Fixed Win32s build not starting on real Win32s.
  * Fixed: scroll range on Win32s is limited to 32k not 65k like NT.
    Now you will be able to scroll up to 8.3 million lines on Win32s.
  * Removed unused functions decreasing exe size slightly.
  * Now when copying a selection, nul chars will be converted to spaces
    so that the selection ca be fully pasted.
  * Fixed: if CP1252 is not installed, ISO8859-1 will be used instead.
  * IMM32.DLL and Global IME are now enabled even on Win32s build.
  * Fixed: hidden/system files will now be editable.
  * Fixed: Status bar window will only be created if it is enabled.
  * Fixed: Now COMCTL32.DLL will be dynamically loaded so that GreenPad
    can work on old versions of Win32s. Status bar will not work if you
    do not have COMCTL32.DLL.

<<What's New in 1.15 (by RamonUnch, 2022/10/21)>>
 < NEW >
  * Now Non-Break Space will be shown with ^ (NBSP character U+00A0 ' ')
    only if you enable the option to show U+3000 (ideographic space).
  * Now the original memory manager is used, this reduces RAM usages and
    improves performances, adds 2KB to the exe though...
  * Now Ctrl+Backspace and Ctrl+Del will delete up to the beginning/end of
    the word like with the standard edit control.
  * Ctrl+U/Ctrl+K will delete all characters until the beginning/end of a line.
  * Now the horizontal wheel is supported to scroll horizontally.

 < FIXED >
  * The Manifest file no longer claims per-monitor DPI-awareness.
  * Now the selection will remain active when user presses the Replace &All
    option in the Find/Replace dialog.
  * Now Latin words will no longer be split when using extended alphabet
    ie: the word éléphant can be selected as a single word instead of 3.
  * Avoid synchronous painting when scrolling.
  * Scroll wheel with small steps are now properly supported.
  * The number of lines to scroll per wheel step now takes into in account
    the system settings instead of being always 3 lines.
  * Now on newer windows versions the a more accurate width will be
    calculated whenever a character is outside the Unicode ranges of the
    selected font thanks to the GetFontUnicodeRanges() function.
  * Scroll bar will no longer be limited to 65K lines on NT3.1/3.5/Win32s.
    The same API is still used but scrolling precision is lost instead
    or the range. The new limit is about 8.9 million lines.
  * Case-insensitive Search will no longer be limited to ASCII.
  * Upper/Lower/Invert case functions will now work on Win32s.

<<What's New in 1.14 (by RamonUnch, 2022/04/28)>>

 < NEW >
  * Now a Win32s build is available. It does not support Unicode display
    but internally everything is stored in UTF-16. So if you open, edit
    and close an Unicode file, you will not "loose" data. Also it handles
    Unicode copy/paste on NT but display is still limited to ANSI.
    I recommend the Win32s version Only if you are still using Win32s.
  * Some error messages boxes will no longer become invisible if you set
    the main GreenPad window topmost ie: Using AltSnap.
  * Now if some files were copied in the clipboard with explorer, the full
    file paths will be pasted in GreenPad on Ctrl+V.

 < CHANGED >
  * Changed Unquote option to Alt+N instead of Alt+Shift+Q that tends to
    change keyboard layout when you miss the Q.
  * Now the Global IME is no longer disabled on Windows 95, it was supposedly
    buggy, but it seems to work just fine... You need to First Install
    Internet Explorer 5.1+ for global IME to be installable.

< FIXED >
  * LTN1 (code page 1252) will no longer be treated internally, this removes
    redundant code and allows proper display of special chars such as 'œ'.
    because the internal handling was not correct.
  * Fixed: Use GetClassInfo() function to determine how to enable status bar
    instead of relying on windows versions.
  * Fixed: When a file contains long lines, GreenPad will no longer freeze.
    Loading can still be slow if you get lines that are more than 1MB.
  * Fixed: Character width is calculated using GetTextExtentPointA function
    on Win32s for character beyond 256, so that the width of all printable
    characters is correctly set.
  * Fixed: Win32s Out of bound array access in the drawing code.
  * Fixed: Now when typing with Greek or Russian keyboards the characters
    will display properly on Win9x
  * Fixed: Now Copy/Paste will handle full Unicode on Win9x.

<<What's New in 1.13 (by RamonUnch, 2022/02/15)>>

  * Replace All is now limited to the current selection (if multi-line).
  * Added Strip First Cahracters (Alt+Z) option, that removes the first
    characted of each line in the selection (inspired from Metapad).
  * Added Quote/Unquote options to add/remove the line commenting
    character(s) to all selected lines. The default commenting char is
    set to '>' for files that have no .kwd associated (ie: *.txt).
    It will be atomatically '//' for C++, ';' for asm etc.
  * You can now select codepage by number directly in the ReOpen dialog.
    It uses the same special codes than from the command line.

<<What's New in 1.12 (by RamonUnch, 2022/01/05)>>

  * Fixed huge crash, regression in 1.11 when disabling word wrapping.
  * Fixed crash caused by MRU on some saved paths (1.10 regression).
  * Improved unicode support (thanks to @rotyam1)
  * Added a bunch of codepage to the list (rotyam1)
  * Include latest revision of chardet.dll (rotyam1)
  * Statusbar should work on build 711 of NT3.5 (rotyam1)

<<What's New in 1.11 (by RamonUnch, 2021/12/21)>>

Christmas release!

Well now it is much better, you even got the statusbar on NT3.1, you no longer
need to upgrade your comctl32 on Win95 and now IMM32 is dynamically imported
at runtime. This means that the NT3.1 build is now the ULTIMATE BUILD.
It runs on all windows version and does not sacrifice any features on newer
windows. The only reason there are two builds is for aesthetic reasons and
for the partial Japanese support possible on NT3.50+

  * If available the IMM32.DLL will be loaded (advanced text services/IME).
  * Thanks to @roytam1, it is now possible to have the statusbar on
    Windows NT3.1, also the "no text display bug" on windows 95 is fixed.
  * Added the /LARGEADDRESSAWARE linker flag to permit up to 4GB ram usage
    when running GreenPad under 64bit edition of Windows.
  * Now the main window is showed before loading the file.
  * Smart indentation supported: keeps indentation level on Return.
  * Added Tab/Shift+Tab to indent/un-indent a multi-line selection.
  * Fix mingw64 builds and provide x64 builds of GreenPad.
  * Fix IID_IMultiLanguage2 definition for encoding auto-detection.
    You no longer need chardet.dll on newer Windows versions.

TODO:
  * Add UAC support for Windows Vista/7/8.X/10/11
  * Find/Replace only in the selection (if any).
  * Improve printer configuration.
  * Fix more bugs, handle low memory situations better
  * Ensure size_t is used over uint where applicable for x64 builds.
  * Optimize memory usage so that larger files can be loaded.

<<What's New in 1.10 (by RamonUnch, 2021/12/03)>>

The purpose of this fork is to make the ultimate exe that will work on
all Windows versions NT3.1-->Win11 while still having MAXIMUM functionality
under later Windows versions.
You can use it also on Win9x, for this you will need UNICOWS.DLL.

For now you have two versions of GreenPad:
GreenPad.exe: English+Japaneese, NT3.5+ compatible version (needs IMM32.DLL)
GPadNT31.exe: English only NT3.1+ compatible version.

  * Support for UNC path, you can edit files with a path longer than 260
    characters, only under Windows NT. Note that the shell is still
    limited to 260 char path so you can drag&drop long file path from
    double-commander or similar software, you can also use the command line.
    but it is not possible to use File->Open to open UNC. Blame Microsoft...
  * Add Upper, Lower and Invert Case options that change the case of selected
    text. Use Alt+U/Alt+L/Alt+I, (like with Metapad).
  * Add Trim trailing spaces option (Alt+W), like with Metapad.
  * Added ESCAPE key to exit GreenPad.
  * Added F12 key to be used for "Save As..", like in MS Office suite.
  * Added F5 to refresh the file (keeps cursor position).
    Use F6 to insert Date&Time.
  * Added new font control option to .lay files.
    - fx to set the width of the font in points (0 = normal)
    - fw to set the weight of the font (0 to 1000), 400=normal, 700=bold...
    - ff to set the 1:italic/2:underlined/4:strikeout font styles.
  * The current font will be used for printing (ttf).
  * Page Setup.. Was added for NT3.51 and later (to set Margin)
  * Fix High DPI issues under Win8.x/10/11 (dpi-aware manifest)
  * Fix crash because of an oob access in ip_parse.c!!!
  * Fix: Use PostMessage instead of SendMessage and avoid to freeze all
    instances of GreenPad when one stops responding.
  * Use CreateWindow instead of CreateStatusWindow, so statubar can be
    included in WinNT3.1 (Needs COMCTL32 v3.5+ though).
  * Dynamically import OLE32.dll so that it is detected at runtime instead
    of compile time (towards ultimate exe).
  * Dynamically import CharNextExA, when using Win95+/NT4+ -> ultimate exe.
  * Re-implement FindWindowEx function using EnumChildWindows for NT3.1.
    So that you can Use Ctrl+Tab there to switch between GreenPad windows.
  * Dynamically import Get/SetScrollInfo for the best scroll support on
    Window NT3.51 and later and fallback to Get/SetScrollPos for NT3.1/3.5.
  * New files can now be created from the Open dialog.
  * Dynamically import GetKeyboardLayout --> ultimate exe.
  * Dynamically import GetShortPathNameW (actually useless...).

TODO:
  * Add UAC support for Windows Vista/7/8.X/10/11
  * Dynamically import IMM32.DLL --> Ultimate exe.
  * Find/Replace only in the selection (if any).
  * Smart indentation option
  * Tab/Shift+Tab to indent/un-indent a block of text.
  * Improve printer configuration.

<<What's New in 1.09 (By roytam1)>>

  * Open/Save dialog, Document Type Menu, MRU Menu fix for NT 3.51
  * UTF-1, UTF-9(1997) support
  * enhanced Multiple Language detection
  * Code page list addition and arrangement
  * Unicode version(provided binary) works in Windows 95/98/Me if system has
    Microsoft Layer for Unicode installed.
  * [100511] add "cs=" .lay font charset value entry support
  * [100511] Use "System" font for NT 3.51's Open/Save dialog.
  * [100514] fix UTF-7/8 support in NT 3.51
  * [100515] NT3.1 and NT 3.5 version merged into GreenPad NT 3.51 repository.
    Binary for Win32s, NT 3.1, NT 3.5 are included in package.
  * [100516] Add chardet.dll charset detector support. Place chardet.dll to
    same place as GreenPad executable to use.
  * chardet.dll is available in separated packages as license is different
    (GreenPad is licensed in NYSL Version 0.9982, but chardet.dll is
    tri-licensed in MPL/GPL/LGPL)
  * [100517] Add UTF-1/UTF-9(1997) BOM support.
  * [100517] Use chardet when size > 80 bytes on NT4/Win95 or newer.
  * [100517] Move EUC/UTF-8 check to last, pervert EUC check on non Japanese
    Edition Windows which leads to misdetection.
  * [100517] textfile: keep U+FEFF in Unicode content, remove BOM only.
  * [100517] Add legacy Chinese Traditional charset support (c_20001.nls -
    c_20005.nls are in install disc, need manual installing them)
  * [100517] Add preliminary support of detect BOM-less UTF-16/32 (BE/LE)
    files. (ideas taken from Footy2)
  * [100517] chardetAutoDetection: add EUC-TW/CNS detection
  * [100520] Fix writing surrogate pairs in UTF-1/UTF-9, fix not converting
    non-BMP code prints to surrogate pair when reading UTF-1/UTF-9
  * [100520] Add SCSU/BOCU-1 support
  * [100523] Fix crash when Quit with Global IME installed
  * [100605] check for valid code page instead of checking OS version for UTF-7/8
  * [100607] fix new file with UNKN encoding when NewfileCharset don't exist in OS
  * [100608] fix Config dialog in NT 3.5, fine tune control positions,
    change full-width katakana to half-width katakana to save space for
    NT 3.x which doesn't have MS PGothic.
  * [100706] add "Save and Exit" menu
  * [100711] add "Discard and Exit" menu
  * [100712] Add preliminary support of print function.
  * known issues: Copies are not working in XP,
    no Page Setup(header/footer/margin/etc.) support (yet?)
  * untested on Windows other than XP. only tested with PrimoPDF and
    XPS Writer virtual printers.
  * [100714] Print: break long lines that cross pages.
  * [100714] Menu: Add "Reconversion" and "IME On/Off" menu.
  * [110228] Add GB18030 codepage support.
  * [110409] Fixed BOCU-1 reader out-of-bound crash when reading invalid
    BOCU-1 stream
  * [110420] Fixed > 65535 lines scrolling on NT 3.51 version (It cannot be
    fixed on NT 3.5/3.1/Win32s version because GetScrollInfo API is not
    available)
  * [110429] Fixed chardet UTF-8 detection


<<What's New in 1.08 (by Kazuhiro Inaba, 2008/07/11)>>

  * Fixed: Several potential access violations bugs
  * Partial support for surrogate pairs (proper rendering and carret moves,
    reading/writing UTF-32 and UTF-8 text beyond BMP).
  * Changed the tab-order of the Find/Replace dialog.
  * Changed the behavior of [Home] and [End] key to be more compatible with NotePad.

<<Keyboard Shortcuts>>

        Ctrl+R   Reopen
  Shift+Ctrl+S   Save as...
        Ctrl+Y   Redo
            F5   Insert date & time

        Ctrl+F   Find
            F3   Find next
      Shift+F3   Find prev
        Ctrl+H   Replace
        Ctrl+J   Jump to line #
        Ctrl+G   Grep

        Ctrl+1   No Wrapping
        Ctrl+2   Wrap by specified width
        Ctrl+3   Wrap by the size of window

       Ctrl+Up   Curosr Up 3 lines
       Ctrl+Dn   Curosr Down 3 lines

  and Windows-common shortcuts ( Ctrl+S to save, Ctrl+C to copy, ... )


<<Q&A>>

  * How to change fonts and colors ?

    In the types/ dir, you'll see some .lay files. Please edit them manually.
      =========================================================
      ct=Color of Text (RGB)
      ck=Color of Keyword
      cb=Color of BackGround
      cc=Color of ControlCharactor
      cn=Color of commeNt
      cl=Color of Line no.
      ft=FonT name
      fw=Font wheight from 1 to 1000 (400=normal, 700=bold...)
      fx=Font width in points (0 means default width)
      ff=Font Flags (1:Italic/2:Underline/4:Strikeout)
      sz=font SiZe (font height in points)
      tb=TaB width
      sc=11000  (Show special chars 0|1 for: [EOF],\n,\t,space,fw-space)
      wp=WraP type (-1: no wrap 0: right edge 1: ww chars )
      ww=Wrap Width
      ws=Wrap Smart (1: wrap at word boundaries, 0: character)
      ln=show LineNo.
      =========================================================
    Year, editing manually, is very inconvinient.
    I'll create GUI configurator someday ...

  * How to create syntax hilighting modes?

    Write you own .kwd files and put them into the types/ directory.
    The format of .kwd files is as follows:
      =========================================================
      1111        # Four Boolean Flags, 0:false 1:true (explained later)
      /*          # beginning symbol for block-comments
      */          # ending symbol for block-comments
      //          # beggining symbol for one-line comments
      auto        # the list of keywords follows...
      bool
      _Bool
      break
      case
      ...
      =========================================================
    The meanings of the four flags are, from left to right:
     - CaseSensitive          (if set to 1, keywords are treated as case-sensitive.)
     - EnableSingleQuotation  (if set to 1, keywords inside '...' is not highlighted.)
     - EnableDoubleQuoatation (if set to 1, keywords inside "..." is not highlighted.)
     - EnableEscapeSequences  (if set to 1, "..\".." is teated as a single string.)
    Usually, 0111 or 1111 is recommended.

  * Which regular expressions can be used?

    Here is the complete list of the regular expressions available in GreenPad:
      =========================================================
      quanitification:
        ?    : 0 or 1
        *    : 0 or more
        +    : 1 or more

      alternation:
        a|b

      grouping(parentheses)

      special escape characters:
        \t   : tab
        \\   : '\' itself
        \[   : '['

      positional match:
        ^  : start of line
        $  : end of line

      character classes:
        [abc]  : matches a single character 'a', 'b', or 'c'
        [^abc] : matches any single character other than 'a', 'b', or 'c'
        [d-h]  : matches 'd', 'e', 'f', 'g', 'h'
        \w     : [0-9a-zA-Z_]
        \W     : [^0-9a-zA-Z_]
        \d     : [0-9]
        \D     : [^0-9]
        \s     : [\t ]
        \S     : [^\t ]
      =========================================================
    There are some limitations:
      * GreenPad does searching line by line, thus
        you cannot search "aaa\nbbb" or something like it.
      * No forward/backward references.
      * No shortest matches (every * is greedy)

  * External Grep Program ?

    You can enter some GUI grep program here. For example,
      C:\Software\Gj\GrepJuice.exe "%D"
    is set in my environment.
      %D is automatically replaced by the current directory
      %F is replaced with the full path of the current file.
      %N is replaced with the name (without path info) of the current file.

  * Command Line Options ?

      greenpad ([-l LineNumber] [-c CharacterSet] filename)*

    For example:
      greenpad -l543 -c932 aaaa.txt
    opens a file named "aaaa.txt" assuming the Shift_JIS encoding,
    and brings its 543rd line to the view area. CharacterSet number
    supported by default is:
       iso-8859-1  = -1
       UTF5        = -2
       UTF8        = -65001
       UTF16BE     = -5
       UTF16LE     = -6
       UTF32BE     = -9
       UTF32LE     = -10
     If you have installed "Language Support" for your Windows,
     the character sets of installed languages become
     readable/writable in GreenPad. You should consult with
     the "area and language option" control panel to get the
     CharacterSet numbet for those languages. Note however that
     for some east asian encodings, special CharacterSet numbers
     are assigned for a technical reason.
       EUC-JP      = -932
       iso-2022-jp = -933
       iso-2022-kr = -950
       iso-2022-cn = -936
       GB2312      = -937

  * How to share GreenPad's configurations between users of same machine?

    Usually, GreenPad saves and loads its configuration for each
    machine user account. However, sometimes you want to use only one
    setting for one GreenPad.exe. (for example, when you have GreenPad
    in an emergency floppy disk and log in different users accounts.)

    In this case, you should add the following two lines to GreenPad.ini
    file:
      [SharedConfig]
      Enable=1
    then GreenPad will be executed in user-independent-settings-mode.


<<Acknowledgements>>

  * The icon image of GreenPad is the work of
    SB( http://homepage3.nifty.com/scriba/ ). Thanks.


<<License>>

  NYSL Version 0.9982  http://www.kmonos.net/nysl/

  A. This software is "Everyone'sWare". It means:
    Anybody who has this software can use it as if you're
    the author.

    A-1. Freeware. No fee is required.
    A-2. You can freely redistribute this software.
    A-3. You can freely modify this software. And the source
        may be used in any software with no limitation.
    A-4. When you release a modified version to public, you
        must publish it with your name.

  B. The author is not responsible for any kind of damages or loss
    while using or misusing this software, which is distributed
    "AS IS". No warranty of any kind is expressed or implied.
    You use AT YOUR OWN RISK.

  C. Copyrighted to k.inaba.

  D. Above three clauses are applied both to source and binary
    form of this software.


---------------------------------------------------------------------------
                                       by k.inaba( http://www.kmonos.net/ )
