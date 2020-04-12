TARGET ?= EdcbPlugIn.tvtp
CPPFLAGS := -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE $(CPPFLAGS)
CXXFLAGS := -Os $(CXXFLAGS)
LDFLAGS := -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)

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
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
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
	$(RM) stdafx.h.gch *.obj $(TARGET)
