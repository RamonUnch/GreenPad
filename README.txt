
=<>
=<> GreenPad ver 1.08+ Source Code
=<>                                                            2008/07/11


  Windows用簡易テキストエディタ GreenPad のソースコードです。
  下にあげるいくつかのC++コンパイラでコンパイルできます。

  Source code for GreenPad - a simple text editor for Windows.
  Can be built by the following compilers.

   - Visual C++ .NET 2005 Express Edition
   - Visual C++ 2003 Toolkit
   - Microsoft Platform SDK for Windows Server 2003 R2 - March 2006 Edition
   - Borland C++ BuilderX
   - Borland C++ Compiler 5.5.1
   - Digital Mars C++ 8.49
   - MinGW (g++ 3.4.2)



:: Visual C++ .NET 2005 ::

  - "kilib.sln" を開いて、「ビルド」メニューの「ソリューションのビルド」

  - Open "kilib.sln" and build the main project.



:: Visual C++ @ Command Prompt (Platform SDK / Toolkit 2003) ::

  - ソースコードのルートディレクトリで "nmake vcc" と打つ

  - Type "nmake vcc" at the root directory of the source archive



:: Visual C++ 6.0 ::

  - 対応しなくなりました。
    作者は確認していませんが、一応 kilib.dsw と kilib.dsp を使えば
    もしかしたらビルドできるかも。

  - No longer supported
    kilib.dsw and kilib.dsp MAY work.



:: Borland C++ @ Command Prompt (5.5.1 / BuilderX) ::

  - ソースコードのルートディレクトリで "make bcc" と打つ。
    必ずBorland製のmakeコマンドを使用すること

  - Type "make bcc" at the root directory of the source archive
    Make sure to use Borland make.



:: Digital Mars C++ ::

  - ソースコードのルートディレクトリで "make dmc" と打つ。
    必ずDigitalmars製のmakeコマンドを使用すること。
    なお、Digital Mars 製のリソースコンパイラは力不足のため、
    リソースのコンパイルに Borland のコンパイラが必要です。
    Borland C++ 5.5 についてくるので入手してください。
    あと、imm32.dll をリンクするため imm32.lib が必要です。
    お手元で生成するか、http://www.kmonos.net/alang/dmc/ から
    入手してください。

  - Type "make dmc" at the root directory of the source archive
    Make sure to use Digitalmars make.
    Since DM's resource compiler is pretty poor, you additionaly
    need Borland's resource compiler to build GreenPad.
    You also need imm32.lib to build. You can generate it by
    coff2omf command or something, or you can download it from
    http://www.kmonos.net/alang/dmc/ .



:: gcc (MinGW) :: 

  - ソースコードのルートディレクトリで "make gcc" と打つ。
    必ずGNU製のmakeコマンドを使用すること。
    MinGW に最初から付属してくるリソースコンパイラ windres は
    日本語対応でないことがあります。MinGW のページから binutils
    を別途ダウンロードして、そちらの windres をご利用下さい。

  - Type "make gcc" at the root directory of the source archive
    Make sure to use GNU make.
    MinGW version of windres (resource compiler) seem not to
    support Japanese resources. So you need to separately download
    binutils from the MinGW page and use the windres in it.



- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

:: ライセンス / License ::

  NYSL Version 0.9982  http://www.kmonos.net/nysl/

  A. 本ソフトウェアは Everyone'sWare です。このソフトを手にした一人一人が、
     ご自分の作ったものを扱うのと同じように、自由に利用することが出来ます。

    A-1. フリーウェアです。作者からは使用料等を要求しません。
    A-2. 有料無料や媒体の如何を問わず、自由に転載・再配布できます。
    A-3. いかなる種類の 改変・他プログラムでの利用 を行っても構いません。
    A-4. 変更したものや部分的に使用したものは、あなたのものになります。
         公開する場合は、あなたの名前の下で行って下さい。

  B. このソフトを利用することによって生じた損害等について、作者は
     責任を負わないものとします。各自の責任においてご利用下さい。

  C. 著作者人格権は K.INABA に帰属します。著作権は放棄します。

  D. 以上の３項は、ソース・実行バイナリの双方に適用されます。


---------------------------------------------------------------------------
                                       by k.inaba( http://www.kmonos.net/ )
