TARGET ?= EpgDataCap_Bon.exe
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0600 -DUSE_WINMAIN_A -DUSE_IBONCAST
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  BonCtrl__BonCtrl.cpp \
  BonCtrl__BonDriverUtil.cpp \
  BonCtrl__CATUtil.cpp \
  BonCtrl__ChSetUtil.cpp \
  BonCtrl__CreatePATPacket.cpp \
  BonCtrl__CreatePMTPacket.cpp \
  BonCtrl__DropCount.cpp \
  BonCtrl__OneServiceUtil.cpp \
  BonCtrl__PacketInit.cpp \
  BonCtrl__PMTUtil.cpp \
  BonCtrl__ScrambleDecoderUtil.cpp \
  BonCtrl__SendTCP.cpp \
  BonCtrl__SendUDP.cpp \
  BonCtrl__ServiceFilter.cpp \
  BonCtrl__TSOut.cpp \
  BonCtrl__WriteTSFile.cpp \
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
  Common__WritePlugInUtil.cpp \
  EpgDataCap_Bon.cpp \
  EpgDataCap_BonDlg.cpp \
  SetDlgApp.cpp \
  SetDlgAppBtn.cpp \
  SetDlgBasic.cpp \
  SetDlgEpg.cpp \
  SetDlgNetwork.cpp \
  SetDlgService.cpp \
  SettingDlg.cpp \
  stdafx.cpp \
  TaskbarList.cpp \
  EpgDataCap_Bon.rc \
  )
	$(CXX) -mwindows -Wl,-s -Wl,--dynamicbase -Wl,--nxcompat -static-libgcc -static-libstdc++ -o $@ $^ -lws2_32 -lole32 -lcomctl32 -ldbghelp -lcomdlg32
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
