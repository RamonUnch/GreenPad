=<> GreenPad ver 1.11
=<> RamonUnch builds 2021/12/21

<<What's This?>>

  GreenPad is a tiny text editor for Windows.

  It aims to be a handy Notepad replacement with minimal but
  complete features, not to be a rich, bloated monsterous
  editor. GreenPad supports:
    * Unicode 2.0
    * Proportional Fonts
    * Syntax Highlighting
    * Searching with Regular Expressions
  while keeping the size of .exe very small (around 130KB!).

  Freeware, distributed under the NYSL licence.
  The original source code is available at: http://www.kmonos.net/lib/gp.en.html
  New source here: https://github.com/RamonUnch/GreenPad
  Supported windows versions: NT3.x/NT4/9x/ME/2000/XP/Vista/7/8.x/10/11

<<What's New in 1.11 (by RamonUnch, 2021/12/21)>>

Christmast release!

Well now it is much better, you even got the statusbar on NT3.1, you no longer
need to upgrade your comctl32 on Win95 and now IMM32 is dynamically imported
at runtime. This means that the NT3.1 build is now the ULTIMATE BUILD.
It runs on all windows version and does not sacrifice any features on newer
windows. The only reason there are two builds is for aestetic reasons and
for the partial japaneese support possible on NT3.50+

  * If available the IMM32.DLL will be loaded (advanced text services/IME).
  * Thanks to @roytam1, it is now possible to have the statusbar on 
    Windows NT3.1, also the "no text display bug" on windows 95 is fixed.
  * Added the /LARGEADDRESSAWARE linker flag to permit up to 4GB ram usage
    when running GreenPad under 64bit edition of Windows.
  * Now the main window is showed before loading the file.
  * Smart indentation supported: keeps indentation level on Return.
  * Added Tab/Shift+Tab to indent/un-indent a multi-line selection.
  * Fix mingw64 builds and provide x64 builds of GreenPad.
  * Fix IID_IMultiLanguage2 definition for encoding autodetection.
    You no longer need chardet.dll on newer Windows versions.

TODO:
  * Add UAC support for Windows Vista/7/8.X/10/11
  * Find/Replace only in the selection (if any).
  * Improve printer configuration.
  * Fix more bugs, handle low memorry situations beter
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
