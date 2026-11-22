TARGETS = EpgDataCap_Bon \
          EpgDataCap3 \
          EpgTimerAdminProxy \
          EpgTimerPlugIn \
          RecName_Macro \
          SendTSTCP \
          Write_Default \
          Write_OneService \
          EdcbPlugIn \
          EpgTimerSrv

all: tools cp_dep $(addsuffix .all, $(TARGETS))
clean: $(addsuffix .clean, $(TARGETS)) clean_dep clean_tools
%.all:
	$(MAKE) -C ../../$*/$* TARGET=../../Document/MinGW/$*
%.clean:
	$(MAKE) -C ../../$*/$* TARGET=$(if $(MSYSTEM),../../Document/MinGW/$*,..\..\Document\MinGW\$*) clean
cp_dep:
ifdef MINGW_PREFIX
	cp -n $(MINGW_PREFIX)/bin/libwinpthread-1.dll .
endif
clean_dep:
	$(RM) libwinpthread-1.dll
asyncbuf.exe: ../../Tools/asyncbuf.c
	$(CC) -municode -Wl,-s -static -DNDEBUG -D_UNICODE -Os -o $@ $<
relayread.exe: ../../Tools/relayread.c
	$(CC) -municode -Wl,-s -static -DNDEBUG -D_UNICODE -Os -o $@ $<
tsidmove.exe: ../../Common/ParseTextInstances.cpp \
              ../../Common/PathUtil.cpp \
              ../../Common/StringUtil.cpp \
              ../../Common/TimeUtil.cpp \
              ../../Tools/tsidmove/tsidmove.cpp
	$(CXX) -municode -Wl,-s -static-libgcc -static-libstdc++ -I../../Tools/tsidmove -DNDEBUG -D_UNICODE -std=c++17 -pedantic-errors -Os -o $@ $+
tspgtxt.exe: ../../BonCtrl/PacketInit.cpp \
             ../../BonCtrl/PATUtil.cpp \
             ../../Common/EpgDataCap3Util.cpp \
             ../../Common/EpgTimerUtil.cpp \
             ../../Common/PathUtil.cpp \
             ../../Common/StringUtil.cpp \
             ../../Common/TimeUtil.cpp \
             ../../Common/TSBuffUtil.cpp \
             ../../Common/TSPacketUtil.cpp \
             ../../Tools/tspgtxt/tspgtxt.cpp
	$(CXX) -municode -Wl,-s -static-libgcc -static-libstdc++ -I../../Tools/tspgtxt -DNDEBUG -D_UNICODE -std=c++17 -pedantic-errors -Os -o $@ $+
tools: asyncbuf.exe relayread.exe tsidmove.exe tspgtxt.exe
clean_tools:
	$(RM) asyncbuf.exe relayread.exe tsidmove.exe tspgtxt.exe

# Extra tools

lua52_URL = https://github.com/xtne6f/lua/archive/00e941f334ea08557a8c0c428203456387d54eb3.tar.gz
lua52_HASH = fc9e0f5a8ae340ff9bcf8f62cf3e8cbad21172b0081d11d0581460d475ccad12
zlib52_URL = https://github.com/xtne6f/lua-zlib/archive/dc3ea17156ad727398ab39501d3cd03e32324165.tar.gz
zlib52_HASH = 34a9eae4aa7df96337545ee48b466887066b5de71e884b38ba4d3cc5ac641acc
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

ifdef MSYSTEM
lua52.tar.gz zlib52.tar.gz b24tovtt.tar.gz psisiarc.tar.gz psisimux.tar.gz tsmemseg.tar.gz tsreadex.tar.gz:
	curl -Lo $@ "$($(basename $(basename $@))_URL)"
	sha256sum $@ | grep -q "$($(basename $(basename $@))_HASH)" || $(RM) $@; [ -f $@ ]
%.tgzdir: %.tar.gz
	$(RM) -r $@
	mkdir $@
	tar xf $< -C $@
lua52.dll zlib52.dll: %.dll: %.tgzdir
	$(MAKE) -C "$(wildcard $</*)" $@
	cp $</*/$@ .
b24tovtt.exe psisiarc.exe psisimux.exe tsmemseg.exe tsreadex.exe: %.exe: %.tgzdir
	$(MAKE) -C "$(wildcard $</*)" CPPFLAGS=-DNDEBUG LDFLAGS="$(if $(filter tsmemseg.exe,$@),,-municode) -static -Wl,-s" TARGET=$@
	cp $</*/$@ .
extra: lua52.dll zlib52.dll b24tovtt.exe psisiarc.exe psisimux.exe tsmemseg.exe tsreadex.exe
clean_extra:
	$(RM) -r lua52.* zlib52.* b24tovtt.* psisiarc.* psisimux.* tsmemseg.* tsreadex.*
else
lua52.tar.gz zlib52.tar.gz b24tovtt.tar.gz psisiarc.tar.gz psisimux.tar.gz tsmemseg.tar.gz tsreadex.tar.gz:
	curl -Lo $@ "$($(basename $(basename $@))_URL)"
	powershell -NoProfile -Command "if((Get-FileHash $@ -Algorithm SHA256).Hash -ne '$($(basename $(basename $@))_HASH)'){rm $@;throw 'Hash error.'}"
%.tgzdir: %.tar.gz
	powershell -NoProfile -Command "rm $@ -Recurse -Force -ErrorAction Ignore;[void](mkdir $@)"
	tar xf $< -C $@
lua52.dll zlib52.dll: %.dll: %.tgzdir
	$(MAKE) -C "$(wildcard $</*)" $@
	powershell -NoProfile -Command "cp $</*/$@ ."
b24tovtt.exe psisiarc.exe psisimux.exe tsmemseg.exe tsreadex.exe: %.exe: %.tgzdir
	$(MAKE) -C "$(wildcard $</*)" CPPFLAGS=-DNDEBUG LDFLAGS="$(if $(filter tsmemseg.exe,$@),,-municode) -static -Wl,-s" TARGET=$@
	powershell -NoProfile -Command "cp $</*/$@ ."
extra: lua52.dll zlib52.dll b24tovtt.exe psisiarc.exe psisimux.exe tsmemseg.exe tsreadex.exe
clean_extra:
	powershell -NoProfile -Command "rm lua52.*,zlib52.*,b24tovtt.*,psisiarc.*,psisimux.*,tsmemseg.*,tsreadex.* -Recurse -Force"
endif
