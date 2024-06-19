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

b24tovtt.tar.gz:
	curl -Lo $@ https://github.com/xtne6f/b24tovtt/archive/refs/tags/master-220402.tar.gz
	sha256sum $@ | grep -q 1f730fc06ac75c6e63e077d9c297d8e5acb8f180c208f90f71d3b75f80ec6f47 || $(RM) $@; [ -f $@ ]
psisiarc.tar.gz:
	curl -Lo $@ https://github.com/xtne6f/psisiarc/archive/refs/tags/master-230324.tar.gz
	sha256sum $@ | grep -q 54d1ab482dc0d1caed80db7858e5c4616f74986f527aabb69d443ec045ad2a02 || $(RM) $@; [ -f $@ ]
psisimux.tar.gz:
	curl -Lo $@ https://github.com/xtne6f/psisimux/archive/refs/tags/master-240131.tar.gz
	sha256sum $@ | grep -q cf93b8048841ccb7df9b9d2592f5e115bd2b5eed824ddf92103b1f325bf004cc || $(RM) $@; [ -f $@ ]
tsmemseg.tar.gz:
	curl -Lo $@ https://github.com/xtne6f/tsmemseg/archive/refs/tags/master-with-d-240608.tar.gz
	sha256sum $@ | grep -q 99bccc04636b0a7de835666a1e7365b48072d0874a489e138ae76589a8661f3a || $(RM) $@; [ -f $@ ]
tsreadex.tar.gz:
	curl -Lo $@ https://github.com/xtne6f/tsreadex/archive/refs/tags/master-240517.tar.gz
	sha256sum $@ | grep -q 1ec21917aa9c1f363d50469d54cef2c121c5086f1bb4eb27efa5297fe1139c44 || $(RM) $@; [ -f $@ ]
b24tovtt psisiarc psisimux tsmemseg tsreadex: b24tovtt.tar.gz psisiarc.tar.gz psisimux.tar.gz tsmemseg.tar.gz tsreadex.tar.gz
	$(RM) -r $@-*
	tar xf $@.tar.gz
	$(MAKE) -C $@-*
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
