
=<>
=<> GreenPad ver 1.08+ Source Code
=<>                                                            2008/07/11


  Windows�p�ȈՃe�L�X�g�G�f�B�^ GreenPad �̃\�[�X�R�[�h�ł��B
  ���ɂ����邢������C++�R���p�C���ŃR���p�C���ł��܂��B

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

  - "kilib.sln" ���J���āA�u�r���h�v���j���[�́u�\�����[�V�����̃r���h�v

  - Open "kilib.sln" and build the main project.



:: Visual C++ @ Command Prompt (Platform SDK / Toolkit 2003) ::

  - �\�[�X�R�[�h�̃��[�g�f�B���N�g���� "nmake vcc" �Ƒł�

  - Type "nmake vcc" at the root directory of the source archive



:: Visual C++ 6.0 ::

  - �Ή����Ȃ��Ȃ�܂����B
    ��҂͊m�F���Ă��܂��񂪁A�ꉞ kilib.dsw �� kilib.dsp ���g����
    ������������r���h�ł��邩���B

  - No longer supported
    kilib.dsw and kilib.dsp MAY work.



:: Borland C++ @ Command Prompt (5.5.1 / BuilderX) ::

  - �\�[�X�R�[�h�̃��[�g�f�B���N�g���� "make bcc" �ƑłB
    �K��Borland����make�R�}���h���g�p���邱��

  - Type "make bcc" at the root directory of the source archive
    Make sure to use Borland make.



:: Digital Mars C++ ::

  - �\�[�X�R�[�h�̃��[�g�f�B���N�g���� "make dmc" �ƑłB
    �K��Digitalmars����make�R�}���h���g�p���邱�ƁB
    �Ȃ��ADigital Mars ���̃��\�[�X�R���p�C���͕͗s���̂��߁A
    ���\�[�X�̃R���p�C���� Borland �̃R���p�C�����K�v�ł��B
    Borland C++ 5.5 �ɂ��Ă���̂œ��肵�Ă��������B
    ���ƁAimm32.dll �������N���邽�� imm32.lib ���K�v�ł��B
    ���茳�Ő������邩�Ahttp://www.kmonos.net/alang/dmc/ ����
    ���肵�Ă��������B

  - Type "make dmc" at the root directory of the source archive
    Make sure to use Digitalmars make.
    Since DM's resource compiler is pretty poor, you additionaly
    need Borland's resource compiler to build GreenPad.
    You also need imm32.lib to build. You can generate it by
    coff2omf command or something, or you can download it from
    http://www.kmonos.net/alang/dmc/ .



:: gcc (MinGW) :: 

  - �\�[�X�R�[�h�̃��[�g�f�B���N�g���� "make gcc" �ƑłB
    �K��GNU����make�R�}���h���g�p���邱�ƁB
    MinGW �ɍŏ�����t�����Ă��郊�\�[�X�R���p�C�� windres ��
    ���{��Ή��łȂ����Ƃ�����܂��BMinGW �̃y�[�W���� binutils
    ��ʓr�_�E�����[�h���āA������� windres �������p�������B

  - Type "make gcc" at the root directory of the source archive
    Make sure to use GNU make.
    MinGW version of windres (resource compiler) seem not to
    support Japanese resources. So you need to separately download
    binutils from the MinGW page and use the windres in it.



- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

:: ���C�Z���X / License ::

  NYSL Version 0.9982  http://www.kmonos.net/nysl/

  A. �{�\�t�g�E�F�A�� Everyone'sWare �ł��B���̃\�t�g����ɂ�����l��l���A
     �������̍�������̂������̂Ɠ����悤�ɁA���R�ɗ��p���邱�Ƃ��o���܂��B

    A-1. �t���[�E�F�A�ł��B��҂���͎g�p������v�����܂���B
    A-2. �L��������}�̂̔@�����킸�A���R�ɓ]�ځE�Ĕz�z�ł��܂��B
    A-3. �����Ȃ��ނ� ���ρE���v���O�����ł̗��p ���s���Ă��\���܂���B
    A-4. �ύX�������̂╔���I�Ɏg�p�������̂́A���Ȃ��̂��̂ɂȂ�܂��B
         ���J����ꍇ�́A���Ȃ��̖��O�̉��ōs���ĉ������B

  B. ���̃\�t�g�𗘗p���邱�Ƃɂ���Đ��������Q���ɂ��āA��҂�
     �ӔC�𕉂�Ȃ����̂Ƃ��܂��B�e���̐ӔC�ɂ����Ă����p�������B

  C. ����Ґl�i���� K.INABA �ɋA�����܂��B���쌠�͕������܂��B

  D. �ȏ�̂R���́A�\�[�X�E���s�o�C�i���̑o���ɓK�p����܂��B


---------------------------------------------------------------------------
                                       by k.inaba( http://www.kmonos.net/ )
