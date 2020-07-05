TARGET ?= EpgDataCap3.dll
CPPFLAGS := -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE $(CPPFLAGS)
CXXFLAGS := -O2 $(CXXFLAGS)
LDFLAGS := -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)

SOURCES_BASE = \
  Common__EpgTimerUtil.cpp \
  Common__PathUtil.cpp \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  Common__TSBuffUtil.cpp \
  Common__TSPacketUtil.cpp \
  AribDescriptor.cpp \
  DecodeUtil.cpp \
  dllmain.cpp \
  EpgDataCap3.cpp \
  EpgDataCap3Main.cpp \
  EpgDBUtil.cpp \
  stdafx.cpp \
  EpgDataCap3.rc

$(TARGET): $(addsuffix .obj,$(SOURCES_BASE) ARIB8CharDecode.cpp) $(basename $(TARGET))_Unicode$(suffix $(TARGET))
	$(CXX) $(LDFLAGS) $(filter %.obj,$^) $(LDLIBS) -o $@
$(basename $(TARGET))_Unicode$(suffix $(TARGET)): $(addsuffix .obj,$(SOURCES_BASE) USE_UNICODE_ARIB8CharDecode.cpp)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
BonCtrl__%.cpp.obj: ../../BonCtrl/%.cpp stdafx.h.gch $(DEP_ALL_H)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
Common__%.cpp.obj: ../../Common/%.cpp stdafx.h.gch $(DEP_ALL_H)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
USE_UNICODE_%.cpp.obj: %.cpp stdafx.h.gch $(DEP_ALL_H) $(CUR_ALL_H)
	$(COMPILE.cpp) -DARIB8CHAR_USE_UNICODE $(OUTPUT_OPTION) $<
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
	$(RM) stdafx.h.gch *.obj $(TARGET) $(basename $(TARGET))_Unicode$(suffix $(TARGET))
