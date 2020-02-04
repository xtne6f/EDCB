TARGET ?= SendTSTCP.dll
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  Common__StringUtil.cpp \
  dllmain.cpp \
  SendTSTCP.cpp \
  SendTSTCPMain.cpp \
  stdafx.cpp \
  SendTSTCP.rc \
  )
	$(CXX) -Wl,-s -Wl,--kill-at -Wl,--dynamicbase -Wl,--nxcompat -static-libgcc -static-libstdc++ -shared -o $@ $^ -lws2_32
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
