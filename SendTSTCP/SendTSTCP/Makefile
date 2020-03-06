TARGET ?= SendTSTCP.dll
CPPFLAGS := -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE $(CPPFLAGS)
CXXFLAGS := -Os $(CXXFLAGS)
LDFLAGS := -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
LDLIBS := -lws2_32 $(LDLIBS)
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)

$(TARGET): $(addsuffix .obj, \
  Common__StringUtil.cpp \
  dllmain.cpp \
  SendTSTCP.cpp \
  SendTSTCPMain.cpp \
  stdafx.cpp \
  SendTSTCP.rc \
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
%.rc.obj: %.rc resource.h targetver.h
	iconv -f UTF-16LE -t UTF-8 $< | $(WINDRES) $(CPPFLAGS) -c 65001 -J rc -O coff $(OUTPUT_OPTION)
clean:
	$(RM) stdafx.h.gch *.obj $(TARGET)
