# -*- cmake -*-
include(Linking)
include(Prebuilt)

option(USE_OPENAL "Enable OpenAL" ON)
if (USE_OPENAL)
  if (USESYSTEMLIBS)
    pkg_check_modules(FREEALUT REQUIRED freealut)
    pkg_check_modules(OPENAL REQUIRED openal)
  else (USESYSTEMLIBS)
    use_prebuilt_binary(openal)
    if(WINDOWS)
      set(OPENAL_LIBRARIES
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/OpenAL32.lib
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/OpenAL32.lib)
      set(FREEALUT_LIBRARIES
          debug ${ARCH_PREBUILT_DIRS_DEBUG}/alut.lib
          optimized ${ARCH_PREBUILT_DIRS_RELEASE}/alut.lib)
    elseif (DARWIN)
      set(OPENAL_LIBRARIES
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libopenal.dylib
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libopenal.dylib)
      set(FREEALUT_LIBRARIES
          debug ${ARCH_PREBUILT_DIRS_DEBUG}/libalut.dylib
          optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libalut.dylib)
    else()
      set(OPENAL_LIBRARIES openal)
      set(FREEALUT_LIBRARIES alut)
    endif()
    set(OPENAL_INCLUDE_DIRS "${LIBS_PREBUILT_DIR}/include/")
  endif (USESYSTEMLIBS)
endif (USE_OPENAL)
