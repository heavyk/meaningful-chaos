DEBUGFLAGS = -g -ggdb -O2
ifeq ($(DEBUG), 1)
	DEBUGFLAGS = -g -ggdb -O0
endif

# find the OS
CXX = g++-9 # TODO: find the best gcc if CXX isn't set
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
CXXFLAGS =  -Wall -Wno-unused-function $(DEBUGFLAGS) -mfma -mavx2 -fPIC -std=c++17 -I external
LDFLAGS = -lm -lc++

# Compile flags for linux / osx
ifeq ($(uname_S),Linux)
	SHOBJ_CFLAGS ?=  -fno-common -g -ggdb
	SHOBJ_LDFLAGS ?= -shared -Bsymbolic -Bsymbolic-functions
else
	MACOSX_VERSION_MIN ?= $(shell defaults read loginwindow SystemVersionStampAsString | awk -F. '{print $$1"."$$2}')
	CXXFLAGS += -mmacosx-version-min=$(MACOSX_VERSION_MIN)
	# CXXFLAGS += -I $(shell brew --prefix)/include -L $(shell brew --prefix)/lib
	SHOBJ_CFLAGS ?= -dynamic -fno-common -g -ggdb
	SHOBJ_LDFLAGS ?= -dylib -exported_symbol _RedisModule_OnLoad -macosx_version_min $(MACOSX_VERSION_MIN)
endif

export

all: meaningful-chaos.so

gmp_test: gmp_test.cpp external/real.hpp $(DEPS)
	$(CXX) $(CXXFLAGS) -o $@ $< -lgmp -lgmpxx -lmpfr

meaningful-chaos.so: meaningful-chaos.o $(DEPS)
	$(LD) $^ -o $@ $(SHOBJ_LDFLAGS) $(LDFLAGS)

redis_date_module.so: date.o $(DEPS)
	$(LD) $^ -o $@ $(SHOBJ_LDFLAGS) $(LDFLAGS)

clean:
	$(RM) meaningful-chaos.o meaningful-chaos.so $(DEPS)
	$(RM) -rf build/
	$(RM) -rf gmp_test

distclean: clean
