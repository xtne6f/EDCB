TARGET ?= EpgTimerSrv.exe
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0600 -DUSE_WINMAIN_A -DUSE_LUA -DNO_CGI -DNO_POPEN -DOPENSSL_API_1_1 #-DEPGDB_STD_WREGEX
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  lua__lapi.c \
  lua__lauxlib.c \
  lua__lbaselib.c \
  lua__lbitlib.c \
  lua__lcode.c \
  lua__lcorolib.c \
  lua__lctype.c \
  lua__ldblib.c \
  lua__ldebug.c \
  lua__ldo.c \
  lua__ldump.c \
  lua__lfunc.c \
  lua__lgc.c \
  lua__linit.c \
  lua__liolib.c \
  lua__llex.c \
  lua__lmathlib.c \
  lua__lmem.c \
  lua__loadlib.c \
  lua__lobject.c \
  lua__lopcodes.c \
  lua__loslib.c \
  lua__lparser.c \
  lua__lstate.c \
  lua__lstring.c \
  lua__lstrlib.c \
  lua__ltable.c \
  lua__ltablib.c \
  lua__ltm.c \
  lua__lundump.c \
  lua__lvm.c \
  lua__lzio.c \
  lua__lua_zlib.c \
  lua_zlib1__adler32.c \
  lua_zlib1__compress.c \
  lua_zlib1__crc32.c \
  lua_zlib1__deflate.c \
  lua_zlib1__infback.c \
  lua_zlib1__inffast.c \
  lua_zlib1__inflate.c \
  lua_zlib1__inftrees.c \
  lua_zlib1__trees.c \
  lua_zlib1__uncompr.c \
  lua_zlib1__zutil.c \
  BonCtrl__CreatePATPacket.cpp \
  BonCtrl__PacketInit.cpp \
  BonCtrl__SendTCP.cpp \
  BonCtrl__SendUDP.cpp \
  Common__CtrlCmdUtil.cpp \
  Common__EpgDataCap3Util.cpp \
  Common__EpgTimerUtil.cpp \
  Common__ParseTextInstances.cpp \
  Common__PathUtil.cpp \
  Common__PipeServer.cpp \
  Common__ReNamePlugInUtil.cpp \
  Common__SendCtrlCmd.cpp \
  Common__ServiceUtil.cpp \
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
  RegExp.cpp \
  ReserveManager.cpp \
  stdafx.cpp \
  SyoboiCalUtil.cpp \
  TunerBankCtrl.cpp \
  UpnpSsdpServer.cpp \
  EpgTimerSrv.rc \
  )
	$(CXX) -mwindows -Wl,-s -Wl,--dynamicbase -Wl,--nxcompat -static-libgcc -static-libstdc++ -o $@ $^ -lws2_32 -lole32 -lcomctl32 -lwinhttp -loleaut32 -lcrypt32 -liphlpapi -luuid -lnetapi32
BonCtrl__%.cpp.obj: ../../BonCtrl/%.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
Common__%.cpp.obj: ../../Common/%.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.cpp.obj: %.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
lua__%.c.obj: lua/%.c
	$(CC) -c $(CPPFLAGS) -I./lua -I./lua/zlib1 -Os -o $@ $<
lua_zlib1__%.c.obj: lua/zlib1/%.c
	$(CC) -c $(CPPFLAGS) -Os -o $@ $<
%.c.obj: %.c
	$(CC) -c $(CPPFLAGS) -Os -o $@ $<
%.rc.obj: %.rc
	iconv -f UTF-16LE -t UTF-8 $< | $(WINDRES) $(CPPFLAGS) -c 65001 -J rc -O coff -o $@
clean:
	$(RM) *.obj $(TARGET)
