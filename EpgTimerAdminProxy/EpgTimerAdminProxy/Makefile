TARGET ?= EpgTimerAdminProxy.exe
CPPFLAGS := -I. -DNDEBUG -DWIN32 -D_WINDOWS -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0600 -DUSE_WINMAIN_A $(CPPFLAGS)
CXXFLAGS := -Os $(CXXFLAGS)
LDFLAGS := -mwindows -Wl,-s,--dynamicbase,--nxcompat -static $(LDFLAGS)
LDLIBS := -lnetapi32 $(LDLIBS)
WINDRES ?= windres

$(TARGET): $(addsuffix .obj, \
  EpgTimerAdminProxy.cpp \
  )
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
%.cpp.obj: %.cpp
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
clean:
	$(RM) *.obj $(TARGET)
