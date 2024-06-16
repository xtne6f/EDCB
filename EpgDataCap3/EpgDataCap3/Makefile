TARGET ?= EpgDataCap3
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)
SOURCES_BASE := \
  Common__EpgTimerUtil.cpp \
  Common__PathUtil.cpp \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  Common__TSBuffUtil.cpp \
  Common__TSPacketUtil.cpp \
  AribDescriptor.cpp \
  DecodeUtil.cpp \
  EpgDataCap3.cpp \
  EpgDataCap3Main.cpp \
  EpgDBUtil.cpp \
  stdafx.cpp

ifeq ($(OS),Windows_NT)
  SOURCES_BASE := $(SOURCES_BASE) dllmain.cpp EpgDataCap3.rc
  TARGETEXT = .dll
  CPPFLAGS := -I. -DNDEBUG -D_WINDOWS -DEPGDATACAP3_EXPORTS -D_UNICODE -DUNICODE $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -O2 $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
else
  TARGETEXT = .so
  CPPFLAGS := -I. -DNDEBUG -DEPGDATACAP3_EXPORTS $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -O2 -fPIC $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s -shared $(LDFLAGS)
endif

$(TARGET)$(TARGETEXT): $(addsuffix .obj,$(SOURCES_BASE) ARIB8CharDecode.cpp) $(TARGET)_Unicode$(TARGETEXT)
	$(CXX) $(LDFLAGS2) $(filter %.obj,$^) $(LDLIBS) -o $@
$(TARGET)_Unicode$(TARGETEXT): $(addsuffix .obj,$(SOURCES_BASE) USE_UNICODE_ARIB8CharDecode.cpp)
	$(CXX) $(LDFLAGS2) $^ $(LDLIBS) -o $@
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
	$(RM) stdafx.h.gch *.obj $(TARGET)$(TARGETEXT) $(TARGET).map $(TARGET)_Unicode$(TARGETEXT) $(TARGET)_Unicode.map
install: $(TARGET)$(TARGETEXT)
	mkdir -p /usr/local/lib/edcb
	install -m 0644 $(TARGET)$(TARGETEXT) /usr/local/lib/edcb
install_u: $(TARGET)$(TARGETEXT)
	mkdir -p /usr/local/lib/edcb
	install -m 0644 $(TARGET)_Unicode$(TARGETEXT) /usr/local/lib/edcb/$(TARGET)$(TARGETEXT)
