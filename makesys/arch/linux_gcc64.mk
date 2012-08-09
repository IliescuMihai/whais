
#Following are linux gcc/g++ specific build settings

ARCH_OBJ_EXT:=.o
ARCH_EXE_EXT:=
ARCH_SHL_PREFIX:=lib
ARCH_SHL_EXT:=.so
ARCH_LIB_PREFIX:=lib
ARCH_LIB_EXT:=.a
CC:=gcc
CXX:=g++
LD:=g++
AR:=ar

ifeq ($(FLAVOR),debug)
CC_FLAGS?=-Wall -Wno-format -g -c -ansi -ftrapv -fvisibility=hidden -fPIC
CXX_FLAGS?=$(CC_FLAGS) -fno-rtti
endif

ifeq ($(FLAVOR),release)
DEFINES+=-DNDEBUG
CC_FLAGS?=-Wall -c -ansi -O3 -fvisibility=hidden -fPIC
CXX_FLAGS?=$(CC_FLAGS) -fno-rtti
endif

DEFINES+=ARCH_LINUX_GCC=1
DEFINES+=INLINE=__inline__


#translate input files
arch_translate_path=$(1)

#set the output object file 
arch_set_output_object=-o $(1)

#set the right option for include headers
arch_add_includes=$(foreach _dir, $(sort $(1)),-I$(_dir))

#set the right option for defines options
arch_add_defines=$(foreach _def, $(sort $($(1)_DEF) $(DEFINES)),-D$(_def))

#set the right libraries directories
arch_add_lib_dirs=$(foreach _dir,$($(1)_LIB_DIR), -L./bin/$(ARCH)/$(_dir))

#set the right libraries adds
arch_handle_import_libs=$(foreach _lib,$($(1)_LIB) $($(1)_SHL),-l$(notdir $(_lib)))

#set the right argument to output executables
arch_set_output_executable=-o ./bin/$(ARCH)/$(1)

#set the right argument to output shared libraries
arch_set_output_sharedlib=-o ./bin/$(ARCH)/$(2)/$(ARCH_SHL_PREFIX)$(1)$(ARCH_SHL_EXT)

#set the right argument to output static libraries
arch_set_output_library=./bin/$(ARCH)/$(2)/$(ARCH_LIB_PREFIX)$(1)$(ARCH_LIB_EXT)

#set the right  flags for the linker
arch_linker_flags= -lgcc -lpthread
arch_shl_linker_flags= -shared -Wl,-soname,lib$(1).so$(2) $(arch_linker_flags)
arch_archiver_flags=rcs

