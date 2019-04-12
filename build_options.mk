# Sources
override SOURCE_DIRS += src lib

# Object directory
OBJECT_DIR ?= obj

# Resulting binary
OUTPUT_FILE ?= bin/modular-forms

# Flags
CXXFLAGS ?= -Wall -Wextra -pedantic-errors -std=c++2a
override CXXFLAGS += -include src/utils/common.h -include src/program/parachute.h -Ilib/include -Isrc
override LDFLAGS += -Llib -lmingw32 -lSDL2main -lSDL2.dll -lfreetype -lopenal32 -lvorbisfile -lvorbisenc -lvorbis -logg -lbz2 -lz

# Targets
.PHONY: debug
debug: override CXXFLAGS += -g -D_GLIBCXX_ASSERTIONS
debug: build

.PHONY: debug_hard
debug_hard: override CXXFLAGS += -g -D_GLIBCXX_DEBUG
debug_hard: build

.PHONY: release
release: override CXXFLAGS += -DNDEBUG -O3
release: override LDFLAGS += -O3 -s -mwindows
release: build

# File-specific flags
obj/lib/glfl.cpp.o: override CXXFLAGS += -O3
obj/lib/implementation.cpp.o: override CXXFLAGS += -O3
