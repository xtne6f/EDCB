TARGET ?= RecName_Macro.dll
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE
WINDRES ?= windres

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
	$(CXX) -Wl,-s -Wl,--kill-at -Wl,--dynamicbase -Wl,--nxcompat -static-libgcc -static-libstdc++ -shared -o $@ $^
BonCtrl__%.cpp.obj: ../../BonCtrl/%.cpp stdafx.h.gch
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
Common__%.cpp.obj: ../../Common/%.cpp stdafx.h.gch
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.cpp.obj: %.cpp stdafx.h.gch
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
stdafx.h.gch: stdafx.h
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.rc.obj: %.rc
	iconv -f UTF-16LE -t UTF-8 $< | $(WINDRES) $(CPPFLAGS) -c 65001 -J rc -O coff -o $@
clean:
	$(RM) stdafx.h.gch *.obj $(TARGET)
