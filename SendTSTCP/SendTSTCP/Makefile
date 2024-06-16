TARGET ?= SendTSTCP
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)
SOURCES := \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  SendTSTCP.cpp \
  SendTSTCPMain.cpp \
  stdafx.cpp

ifeq ($(OS),Windows_NT)
  SOURCES := $(SOURCES) dllmain.cpp SendTSTCP.rc
  TARGETEXT = .dll
  CPPFLAGS := -I. -DNDEBUG -D_WINDOWS -DSENDTSTCP_EXPORTS -D_UNICODE -DUNICODE $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
  LDLIBS := -lws2_32 $(LDLIBS)
else
  SOURCES := $(SOURCES) Common__PathUtil.cpp
  TARGETEXT = .so
  CPPFLAGS := -I. -DNDEBUG -DSENDTSTCP_EXPORTS $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os -fPIC -pthread $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s -shared -pthread $(LDFLAGS)
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
	mkdir -p /usr/local/lib/edcb
	install -m 0644 $(TARGET)$(TARGETEXT) /usr/local/lib/edcb
