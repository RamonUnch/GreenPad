
message:
	-@echo  Specify one of the following toolset as the target of make:
	-@echo    make gcc   (for MinGW - i386 build)
	-@echo    make gcc64 (for MinGW64 - x86_64 build)
	-@echo    make dmc   (for DigitalMars C++)
	-@echo    make bcc   (for Borland C++ Compilers)
	-@echo    make vcc   (for Microsoft Visual C++)
	-@echo  Please make sure that the "make" program you're using is
	-@echo  the one from the toolset.
	-@echo  (GNU make for gcc, nmake for vcc, ... etc.)

clean:
	-@rmdir /Q /S obj        2> nul
	-@del   /Q release\*.exe 2> nul

############################################################################

DMAK = make# Hey, why no $(MAKE)????

gcc:
	$(MAKE) -f Makefiles/gcc.mak
dmc:
	$(DMAK) -f Makefiles/dmc.mak
vcc:
	$(MAKE) -f Makefiles/vcc.mak
bcc:
	$(MAKE) -f Makefiles/bcc.mak
gcc64:
	$(MAKE) -f Makefiles/gcc64.mak