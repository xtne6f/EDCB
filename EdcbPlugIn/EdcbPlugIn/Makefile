TARGET ?= EdcbPlugIn.tvtp
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE #-DSEND_PIPE_TEST
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  BonCtrl__CATUtil.cpp \
  BonCtrl__CreatePATPacket.cpp \
  BonCtrl__DropCount.cpp \
  BonCtrl__PMTUtil.cpp \
  BonCtrl__SendTCP.cpp \
  BonCtrl__ServiceFilter.cpp \
  Common__CtrlCmdUtil.cpp \
  Common__EpgDataCap3Util.cpp \
  Common__EpgTimerUtil.cpp \
  Common__ParseTextInstances.cpp \
  Common__PathUtil.cpp \
  Common__PipeServer.cpp \
  Common__SendCtrlCmd.cpp \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  Common__TSBuffUtil.cpp \
  Common__TSPacketUtil.cpp \
  EdcbPlugIn.cpp \
  stdafx.cpp \
  )
	$(CXX) -Wl,-s -Wl,--kill-at -Wl,--dynamicbase -Wl,--nxcompat -static-libgcc -static-libstdc++ -shared -o $@ $^
BonCtrl__%.cpp.obj: ../../BonCtrl/%.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
Common__%.cpp.obj: ../../Common/%.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.cpp.obj: %.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.rc.obj: %.rc
	iconv -f UTF-16LE -t UTF-8 $< | $(WINDRES) $(CPPFLAGS) -c 65001 -J rc -O coff -o $@
clean:
	$(RM) *.obj $(TARGET)
