TARGET ?= EpgDataCap_Bon
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)
SOURCES := \
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
  BonCtrl__ServiceFilter.cpp \
  BonCtrl__TSOut.cpp \
  BonCtrl__WriteTSFile.cpp \
  Common__CtrlCmdUtil.cpp \
  Common__EpgDataCap3Util.cpp \
  Common__EpgTimerUtil.cpp \
  Common__MessageManager.cpp \
  Common__ParseTextInstances.cpp \
  Common__PathUtil.cpp \
  Common__PipeServer.cpp \
  Common__SendCtrlCmd.cpp \
  Common__SendTSTCPDllUtil.cpp \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  Common__TSBuffUtil.cpp \
  Common__TSPacketUtil.cpp \
  Common__WritePlugInUtil.cpp \
  AppSetting.cpp \
  EpgDataCap_Bon.cpp \
  stdafx.cpp

ifeq ($(OS),Windows_NT)
  SOURCES := $(SOURCES) \
    Common__IniUtil.cpp \
    Common__StackTrace.cpp \
    EpgDataCap_BonDlg.cpp \
    SetDlgApp.cpp \
    SetDlgAppBtn.cpp \
    SetDlgBasic.cpp \
    SetDlgEpg.cpp \
    SetDlgNetwork.cpp \
    SetDlgService.cpp \
    SettingDlg.cpp \
    TaskbarList.cpp \
    EpgDataCap_Bon.rc
  TARGETEXT = .exe
  CPPFLAGS := -I. -DNDEBUG -D_WINDOWS -DEPGDATACAP_BON_APPLICATION -D_UNICODE -DUNICODE $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os $(CXXFLAGS)
  LDFLAGS2 = -municode -mwindows -Wl,-s,--dynamicbase,--nxcompat -static-libgcc -static-libstdc++ $(LDFLAGS)
  LDLIBS := -lole32 -lcomctl32 -ldbghelp -lcomdlg32 $(LDLIBS)
else
  SOURCES := $(SOURCES) EpgDataCap_BonMin.cpp
  TARGETEXT =
  CPPFLAGS := -I. -DNDEBUG -DEPGDATACAP_BON_APPLICATION $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os -pthread $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s -pthread $(LDFLAGS)

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
