TARGETS = EpgDataCap_Bon \
          EpgDataCap3 \
          RecName_Macro \
          SendTSTCP \
          Write_Default \
          EpgTimerSrv

all: $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS))
install: $(addsuffix .install, $(TARGETS))
install_u: $(subst EpgDataCap3.install, EpgDataCap3.install_u, $(addsuffix .install, $(TARGETS)))
%.all:
	$(MAKE) -C ../../$*/$*
%.clean:
	$(MAKE) -C ../../$*/$* clean
%.install:
	$(MAKE) -C ../../$*/$* install
%.install_u:
	$(MAKE) -C ../../$*/$* install_u
