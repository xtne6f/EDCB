TARGETS = EpgDataCap_Bon.exe \
          EpgDataCap3.dll \
          EpgTimerAdminProxy.exe \
          EpgTimerPlugIn.tvtp \
          RecName_Macro.dll \
          SendTSTCP.dll \
          Write_Default.dll \
          EpgTimerSrv.exe

all: cp_dep $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS)) rm_dep
%.all:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$*
%.clean:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$* clean
cp_dep:
	cp -n $(MINGW_PREFIX)/bin/libwinpthread-1.dll .
rm_dep:
	$(RM) libwinpthread-1.dll
