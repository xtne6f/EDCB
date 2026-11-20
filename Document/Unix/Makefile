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
asyncbuf: ../../Tools/asyncbuf.c
	$(CC) -Wl,-s -DNDEBUG -Os -o $@ $< -pthread
relayread: ../../Tools/relayread.c
	$(CC) -Wl,-s -DNDEBUG -Os -o $@ $<
tsidmove-edcb: ../../Common/ParseTextInstances.cpp \
               ../../Common/PathUtil.cpp \
               ../../Common/StringUtil.cpp \
               ../../Common/TimeUtil.cpp \
               ../../Tools/tsidmove/tsidmove.cpp
	$(CXX) -Wl,-s -I../../Tools/tsidmove -DNDEBUG -std=c++17 -pedantic-errors -Os -o $@ $+
tspgtxt: ../../BonCtrl/PacketInit.cpp \
         ../../BonCtrl/PATUtil.cpp \
         ../../Common/EpgDataCap3Util.cpp \
         ../../Common/EpgTimerUtil.cpp \
         ../../Common/PathUtil.cpp \
         ../../Common/StringUtil.cpp \
         ../../Common/TimeUtil.cpp \
         ../../Common/TSBuffUtil.cpp \
         ../../Common/TSPacketUtil.cpp \
         ../../Tools/tspgtxt/tspgtxt.cpp
	$(CXX) -Wl,-s -I../../Tools/tspgtxt -DNDEBUG -std=c++17 -pedantic-errors -Os -o $@ $+
tools: asyncbuf relayread tsidmove-edcb tspgtxt
clean_tools:
	$(RM) asyncbuf relayread tsidmove-edcb tspgtxt
install_tools: tools
	install asyncbuf relayread tsidmove-edcb tspgtxt /usr/local/bin

# Extra tools

b24tovtt_URL = https://github.com/xtne6f/b24tovtt/archive/refs/tags/master-220402.tar.gz
b24tovtt_HASH = 1f730fc06ac75c6e63e077d9c297d8e5acb8f180c208f90f71d3b75f80ec6f47
psisiarc_URL = https://github.com/xtne6f/psisiarc/archive/refs/tags/master-230324.tar.gz
psisiarc_HASH = 54d1ab482dc0d1caed80db7858e5c4616f74986f527aabb69d443ec045ad2a02
psisimux_URL = https://github.com/xtne6f/psisimux/archive/refs/tags/master-260422.tar.gz
psisimux_HASH = cc6996370123042ba6a1757d912a52de0cd97b3645fa32b5799f112365fa7b25
ifeq ($(TSMEMSEG_WITH_D)$(if $(TSMEMSEG_WITHOUT_D),,y),)
  tsmemseg_URL = https://github.com/xtne6f/tsmemseg/archive/refs/tags/master-260611.tar.gz
  tsmemseg_HASH = a006670c7471ffd8b06090043d78f320de5af7c32e53ec195f6a4638f0af476f
else
  tsmemseg_URL = https://github.com/xtne6f/tsmemseg/archive/refs/tags/master-with-d-260611.tar.gz
  tsmemseg_HASH = b0fff07fc7858aafdeac249ef628769feaffd73c3d8f4ba1d2a1996d52bf2dd2
endif
tsreadex_URL = https://github.com/xtne6f/tsreadex/archive/refs/tags/master-260428.tar.gz
tsreadex_HASH = a60fb7aa220b4dff15b384ca28e76181de59148e8be487a728a39165dee50520

b24tovtt.tar.gz psisiarc.tar.gz psisimux.tar.gz tsmemseg.tar.gz tsreadex.tar.gz:
	curl -Lo $@ "$($(basename $(basename $@))_URL)"
	sha256sum $@ | grep -q "$($(basename $(basename $@))_HASH)" || $(RM) $@; [ -f $@ ]
b24tovtt psisiarc psisimux tsmemseg tsreadex: %: %.tar.gz
	$(RM) -r $@-*
	tar xf $<
	$(MAKE) -C $@-* CPPFLAGS=-DNDEBUG
	cp $@-*/$@ .
extra: b24tovtt psisiarc psisimux tsmemseg tsreadex
clean_extra:
	$(RM) -r b24tovtt* psisiarc* psisimux* tsmemseg* tsreadex*
install_extra: extra
	install b24tovtt psisiarc psisimux tsmemseg tsreadex /usr/local/bin

setup_ini:
	[ -f /var/local/edcb/Bitrate.ini ] || iconv -f CP932 -t UTF-8 ../../ini/Bitrate.ini | tr -d '\r' >/var/local/edcb/Bitrate.ini
	[ -f /var/local/edcb/BonCtrl.ini ] || iconv -f CP932 -t UTF-8 ../../ini/BonCtrl.ini | tr -d '\r' | sed 's/\.dll$$/.so/' >/var/local/edcb/BonCtrl.ini
	[ -f /var/local/edcb/ContentTypeText.txt ] || tr -d '\r' <../../ini/ContentTypeText.txt >/var/local/edcb/ContentTypeText.txt
	mkdir -p /var/local/edcb/HttpPublic
	cp -Rn ../../ini/HttpPublic/index.html ../../ini/HttpPublic/favicon.ico ../../ini/HttpPublic/legacy /var/local/edcb/HttpPublic/
	if [ ! -f /var/local/edcb/EpgTimerSrv.ini ]; then \
	  echo '[SET]' >/var/local/edcb/EpgTimerSrv.ini; \
	  echo 'EnableHttpSrv=2' >>/var/local/edcb/EpgTimerSrv.ini; \
	  echo 'HttpAccessControlList=+127.0.0.1,+::1,+::ffff:127.0.0.1' >>/var/local/edcb/EpgTimerSrv.ini; \
	fi
