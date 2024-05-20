TARGETS = EpgDataCap_Bon \
          EpgDataCap3 \
          RecName_Macro \
          SendTSTCP \
          Write_Default \
          EpgTimerSrv

all: tools $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS)) clean_tools
install: install_tools $(addsuffix .install, $(TARGETS))
install_u: install_tools $(subst EpgDataCap3.install, EpgDataCap3.install_u, $(addsuffix .install, $(TARGETS)))
%.all:
	$(MAKE) -C ../../$*/$*
%.clean:
	$(MAKE) -C ../../$*/$* clean
%.install:
	$(MAKE) -C ../../$*/$* install
%.install_u:
	$(MAKE) -C ../../$*/$* install_u
asyncbuf: ../../ini/Tools/asyncbuf.c
	$(CC) -Wl,-s -Os -o $@ $< -pthread
relayread: ../../ini/Tools/relayread.c
	$(CC) -Wl,-s -Os -o $@ $<
tsidmove-edcb: ../../Common/ParseTextInstances.cpp \
               ../../Common/PathUtil.cpp \
               ../../Common/StringUtil.cpp \
               ../../Common/TimeUtil.cpp \
               ../../ini/Tools/tsidmove/tsidmove.cpp
	$(CXX) -Wl,-s -I../../ini/Tools/tsidmove -std=c++17 -pedantic-errors -Os -o $@ $+
tools: asyncbuf relayread tsidmove-edcb
clean_tools:
	$(RM) asyncbuf relayread tsidmove-edcb
install_tools: tools
	install asyncbuf relayread tsidmove-edcb /usr/local/bin
