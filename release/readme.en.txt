

=<>
=<> GreenPad ver 1.08+
=<>                                                            2008/07/11


<<What's This?>>

  GreenPad is a tiny text editor for Windows.

  It aims to be a handy Notepad replacement with minimal but
  complete features, not to be a rich, bloated monsterous
  editor. GreenPad supports:
    * Unicode 2.0
    * Proportional Fonts
    * Syntax Highlighting
    * Searching with Regular Expressions
  while keeping the size of .exe very small (around 50KB!).

  Freeware, distributed under the NYSL licence.
  The source code is available at: http://www.kmonos.net/lib/gp.en.html


<<What's New in 1.08>>

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
      sz=font SiZe
      tb=TaB width
      sc=11000
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
