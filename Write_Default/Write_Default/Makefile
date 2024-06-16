TARGET ?= Write_Default
OBJCOPY ?= objcopy
WINDRES ?= windres
DEP_ALL_H := $(wildcard ../../BonCtrl/*.h ../../Common/*.h)
CUR_ALL_H := $(wildcard *.h)
SOURCES := \
  Common__PathUtil.cpp \
  Common__StringUtil.cpp \
  stdafx.cpp \
  WriteMain.cpp \
  Write_Default.cpp

ifeq ($(OS),Windows_NT)
  SOURCES := $(SOURCES) dllmain.cpp SettingDlg.cpp Write_Default.rc
  TARGETEXT = .dll
  CPPFLAGS := -I. -DNDEBUG -D_WINDOWS -DWRITE_DEFAULT_EXPORTS -D_UNICODE -DUNICODE $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s,--dynamicbase,--nxcompat,--kill-at -static-libgcc -static-libstdc++ -shared $(LDFLAGS)
else
  SOURCES := $(SOURCES) embed.txt
  TARGETEXT = .so
  CPPFLAGS := -I. -DNDEBUG -DWRITE_DEFAULT_EXPORTS $(CPPFLAGS)
  CXXFLAGS := -std=c++17 -pedantic-errors -Os -fPIC -pthread $(CXXFLAGS)
  LDFLAGS2 = -Wl,-s -shared -pthread $(LDFLAGS)
  # "-m" emulation may be required in some environments (e.g."-m elf_x86_64")
  EMBED_TXT_LDFLAGS2 = -r -b binary $(EMBED_TXT_LDFLAGS)
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
embed.txt.b.obj: embed.txt
	$(LD) $(EMBED_TXT_LDFLAGS2) $< -o $@
embed.txt.obj: embed.txt.b.obj
	$(OBJCOPY) --rename-section .data=.embed,readonly,contents --add-section .note.GNU-stack=/dev/null $< $@
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
