TARGET ?= RecName_Macro.dll
CPPFLAGS := -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE $(CPPFLAGS)
CXXFLAGS := -Os $(CXXFLAGS)
LDFLAGS := -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)

$(TARGET): $(addsuffix .obj, \
  Common__EpgTimerUtil.cpp \
  Common__PathUtil.cpp \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  ConvertMacro2.cpp \
  dllmain.cpp \
  RecName_Macro.cpp \
  SettingDlg.cpp \
  stdafx.cpp \
  RecName_Macro.rc \
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
