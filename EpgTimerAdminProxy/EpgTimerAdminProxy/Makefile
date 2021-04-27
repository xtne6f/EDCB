TARGET ?= EpgTimerAdminProxy
CPPFLAGS := -I. -DNDEBUG -D_WINDOWS -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0600 $(CPPFLAGS)
CXXFLAGS := -std=c++17 -pedantic-errors -Os $(CXXFLAGS)
LDFLAGS2 = -municode -mwindows -Wl,-s,--dynamicbase,--nxcompat -static $(LDFLAGS)
LDLIBS := -lnetapi32 $(LDLIBS)
WINDRES ?= windres

$(TARGET).exe: $(addsuffix .obj, \
  EpgTimerAdminProxy.cpp \
  )
	$(CXX) $(LDFLAGS2) $^ $(LDLIBS) -o $@
%.cpp.obj: %.cpp
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<
clean:
	$(RM) *.obj $(TARGET).exe $(TARGET).map
