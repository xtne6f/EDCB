TARGET ?= EpgTimerSrv
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h *.inl)
SOURCES := \
  BonCtrl__CreatePATPacket.cpp \
  BonCtrl__PacketInit.cpp \
  Common__CtrlCmdUtil.cpp \
  Common__EpgDataCap3Util.cpp \
  Common__EpgTimerUtil.cpp \
  Common__MessageManager.cpp \
  Common__ParseTextInstances.cpp \
  Common__PathUtil.cpp \
  Common__PipeServer.cpp \
  Common__ReNamePlugInUtil.cpp \
  Common__SendCtrlCmd.cpp \
  Common__SendTSTCPDllUtil.cpp \
  Common__StringUtil.cpp \
  Common__TCPServer.cpp \
  Common__TimeShiftUtil.cpp \
  Common__TimeUtil.cpp \
  Common__TSPacketUtil.cpp \
  BatManager.cpp \
  civetweb.c \
  EpgDBManager.cpp \
  EpgTimerSrv.cpp \
  EpgTimerSrvMain.cpp \
  EpgTimerSrvSetting.cpp \
  HttpServer.cpp \
  NotifyManager.cpp \
  ReserveManager.cpp \
  stdafx.cpp \
  TunerBankCtrl.cpp \
  UpnpSsdpServer.cpp

ifeq ($(OS),Windows_NT)
  SOURCES := $(SOURCES) \
    Common__IniUtil.cpp \
    Common__ServiceUtil.cpp \
    Common__StackTrace.cpp EpgTimerTask.cpp \
    RegExp.cpp \
    SyoboiCalUtil.cpp \
    EpgTimerSrv.rc
  TARGETEXT = .exe
  CPPFLAGS := -I. -DNDEBUG -D_WINDOWS -DEPGTIMERSRV_APPLICATION -D_UNICODE -DUNICODE -DUSE_LUA -DUSE_IPV6 -DNO_CGI -DNO_POPEN -DOPENSSL_API_3_0 $(CPPFLAGS)
  #CPPFLAGS += -DEPGDB_STD_WREGEX
  CFLAGS := -Os $(CFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os $(CXXFLAGS)
  LDFLAGS2 = -municode -mwindows -Wl,-s,--dynamicbase,--nxcompat -static-libgcc -static-libstdc++ $(LDFLAGS)
  LDLIBS := -lws2_32 -lole32 -lcomctl32 -lwinhttp -loleaut32 -lcrypt32 -liphlpapi -luuid -lnetapi32 -ldbghelp $(LDLIBS)

  # Lazy loading fails with segfault if LUA_BUILD_AS_DLL defined.
  LDLIBS := -L. -llua52-mingw-lazy-$(or $(MSYSTEM_CARCH),$(findstring x86_64,$(MAKE_HOST)),i686) $(LDLIBS)

  ifndef MSYSTEM
    # Assume non-pthread threading model
    CPPFLAGS += -DMUST_IMPLEMENT_CLOCK_GETTIME
  endif
else
  TARGETEXT =
  CPPFLAGS := -I. -DNDEBUG -DEPGTIMERSRV_APPLICATION -DUSE_LUA -DUSE_IPV6 -DNO_CGI -DNO_POPEN -DOPENSSL_API_3_0 $(CPPFLAGS)
  CFLAGS := -Os -pthread $(CFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os -pthread $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s -pthread $(LDFLAGS)
  LDLIBS := -L. -llua5.2 $(LDLIBS)

  ifneq ($(shell uname),Linux)
    # For systems that do not have procfs
    CPPFLAGS := -DPATH_UTIL_FIX_SELF_EXE=L\"/usr/local/bin/$(TARGET)\" $(CPPFLAGS)
  endif
endif

$(TARGET)$(TARGETEXT): $(addsuffix .obj,$(SOURCES))
	$(CXX) $(LDFLAGS2) $^ $(LDLIBS) -o $@
BonCtrl__%.cpp.obj: ../../BonCtrl/%.cpp stdafx.h.gch $(DEP_ALL_H)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
Common__%.cpp.obj: ../../Common/%.cpp stdafx.h.gch $(DEP_ALL_H)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
%.cpp.obj: %.cpp stdafx.h.gch $(DEP_ALL_H) $(CUR_ALL_H)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
stdafx.h.gch: stdafx.h ../../Common/Common.h targetver.h
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
%.c.obj: %.c $(CUR_ALL_H)
	$(COMPILE.c) $(OUTPUT_OPTION) $<
%.rc.obj: %.rc.u.obj resource.h targetver.h
	$(WINDRES) $(CPPFLAGS) -c 65001 -J rc -O coff $(OUTPUT_OPTION) $<
%.rc.u.obj: %.rc
ifdef MSYSTEM
	iconv -f UTF-16LE -t UTF-8 $< >$@
else
	powershell -NoProfile -Command "&{cat $<|Out-File -Encoding UTF8 $@}"
endif
clean:
	$(RM) stdafx.h.gch *.obj $(TARGET)$(TARGETEXT) $(TARGET).map
install: $(TARGET)$(TARGETEXT)
	install $(TARGET)$(TARGETEXT) /usr/local/bin
