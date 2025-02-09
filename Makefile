
message:
	-@echo  Specify one of the following toolset as the target of make:
	-@echo    make gcc   (for MinGW - i386 build)
	-@echo    make gcc32s(for MinGW - i386 build Win32s)
	-@echo    make gcc32sl(for MinGW - i386 build Win32s light)
	-@echo    make gccprf(for MinGW - i386 build profiling)
	-@echo    make gccdb (for MinGW - i386 bebug build)
	-@echo    make gcc64 (for MinGW64 - x86_64 build)
	-@echo    make clang (for LLVM Clang - i386 build)
	-@echo    make dmc   (for DigitalMars C++)
	-@echo    make bcc   (for Borland C++ Compilers)
	-@echo    make vcc   (for Microsoft Visual C++)
	-@echo    make wcc386(for OpenWatcom C++ i386 build)
	-@echo    make wccAXP(for OpenWatcom C++ Aplha build)
	-@echo  Please make sure that the "make" program you're using is
	-@echo  the one from the toolset.
	-@echo  (GNU make for gcc, nmake for vcc, ... etc.)

clean:
	-@rmdir /Q /S obj        2> nul
	-@del   /Q release\*.exe 2> nul

############################################################################

DMAK = make# Hey, why no $(MAKE)????

gcc:
	$(MAKE) -f Makefiles/gcc.mak -j2
dmc:
	$(DMAK) -f Makefiles/dmc.mak
vcc:
	$(MAKE) -f Makefiles/vcc.mak
bcc:
	$(MAKE) -f Makefiles/bcc.mak
gcc64:
	$(MAKE) -f Makefiles/gcc64.mak -j2
gccdb:
	$(MAKE) -f Makefiles/gccdb.mak -j2
gcc32s:
	$(MAKE) -f Makefiles/gcc32s.mak -j2
gcc32sl:
	$(MAKE) -f Makefiles/gcc32sl.mak -j2
gccprf:
	$(MAKE) -f Makefiles/gccprf.mak -j2
clang:
	$(MAKE) -f Makefiles/clang.mak -j2
wcc386:
	$(MAKE) -f Makefiles/wcc386.mak
wccaxp:
	$(MAKE) -f Makefiles/wccAXP.mak
gccO2:
	$(MAKE) -f Makefiles/gccO2.mak -j2
