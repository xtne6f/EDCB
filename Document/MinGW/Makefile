TARGETS = EpgDataCap_Bon.exe \
          EpgDataCap3.dll \
          EpgTimerAdminProxy.exe \
          EpgTimerPlugIn.tvtp \
          RecName_Macro.dll \
          SendTSTCP.dll \
          Write_Default.dll \
          EpgTimerSrv.exe

all: $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS))
%.all:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$*
%.clean:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$* clean
