TARGETS = EpgDataCap_Bon.exe \
          EpgDataCap3.dll \
          EpgTimerAdminProxy.exe \
          EpgTimerPlugIn.tvtp \
          RecName_Macro.dll \
          SendTSTCP.dll \
          Write_Default.dll \
          Write_OneService.dll \
          EdcbPlugIn.tvtp

# EpgTimerSrv.exe is also buildable, but has some problems about civetweb.c. Don't use.

all: $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS))
%.all:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$*
%.clean:
	$(MAKE) -C ../../$(basename $*)/$(basename $*) TARGET=../../Document/MinGW/$* clean
