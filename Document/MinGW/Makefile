TARGETS = EpgDataCap_Bon.exe \
          EpgDataCap3.dll \
          EpgTimerAdminProxy.exe \
          EpgTimerPlugIn.tvtp \
          RecName_Macro.dll \
          SendTSTCP.dll \
          Write_Default.dll \
          Write_OneService.dll \
          EdcbPlugIn.tvtp \
          EpgTimerSrv.exe

all: readex.exe relayread.exe tsidmove.exe cp_dep $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS)) rm_dep rm_tools
%.all:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$*
%.clean:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=$(if $(MSYSTEM),../../Document/MinGW/$*,..\..\Document\MinGW\$*) clean
cp_dep:
ifdef MINGW_PREFIX
	cp -n $(MINGW_PREFIX)/bin/libwinpthread-1.dll .
endif
rm_dep:
	$(RM) libwinpthread-1.dll
readex.exe: ../../ini/Tools/readex.c
	$(CC) -municode -Wl,-s -static -D_UNICODE -Os -o $@ $<
relayread.exe: ../../ini/Tools/relayread.c
	$(CC) -municode -Wl,-s -static -D_UNICODE -Os -o $@ $<
tsidmove.exe: ../../Common/ParseTextInstances.cpp \
              ../../Common/PathUtil.cpp \
              ../../Common/StringUtil.cpp \
              ../../Common/TimeUtil.cpp \
              ../../ini/Tools/tsidmove/tsidmove.cpp
	$(CXX) -municode -Wl,-s -static-libgcc -static-libstdc++ -I../../ini/Tools/tsidmove -D_UNICODE -Os -o $@ $+
rm_tools:
	$(RM) readex.exe relayread.exe tsidmove.exe
