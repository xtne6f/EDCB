TARGET ?= EpgTimerAdminProxy.exe
CPPFLAGS = -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0600 -DUSE_WINMAIN_A
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  EpgTimerAdminProxy.cpp \
  )
	$(CXX) -mwindows -Wl,-s -Wl,--dynamicbase -Wl,--nxcompat -static -o $@ $^ -lnetapi32
BonCtrl__%.cpp.obj: ../../BonCtrl/%.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
Common__%.cpp.obj: ../../Common/%.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.cpp.obj: %.cpp
	$(CXX) -c $(CPPFLAGS) -finput-charset=CP932 -Os -o $@ $<
%.rc.obj: %.rc
	iconv -f UTF-16LE -t UTF-8 $< | $(WINDRES) $(CPPFLAGS) -c 65001 -J rc -O coff -o $@
clean:
	$(RM) *.obj $(TARGET)
