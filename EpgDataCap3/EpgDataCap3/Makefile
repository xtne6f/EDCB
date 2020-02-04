TARGET ?= EpgDataCap3.dll
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE #-DARIB8CHAR_USE_UNICODE
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  Common__EpgTimerUtil.cpp \
  Common__PathUtil.cpp \
  Common__StringUtil.cpp \
  Common__TimeUtil.cpp \
  Common__TSBuffUtil.cpp \
  Common__TSPacketUtil.cpp \
  ARIB8CharDecode.cpp \
  AribDescriptor.cpp \
  DecodeUtil.cpp \
  dllmain.cpp \
  EpgDataCap3.cpp \
  EpgDataCap3Main.cpp \
  EpgDBUtil.cpp \
  stdafx.cpp \
  EpgDataCap3.rc \
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
