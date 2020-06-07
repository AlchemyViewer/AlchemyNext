# -*- cmake -*-

include(Variables)
include(FreeType)
include(GLH)

if (BUILD_VULKAN)
    set(LLRENDER_INCLUDE_DIRS
        ${LIBS_OPEN_DIR}/gzrender
        ${GLH_INCLUDE_DIR}
    )
else ()
    set(LLRENDER_INCLUDE_DIRS
        ${LIBS_OPEN_DIR}/llrender
        ${GLH_INCLUDE_DIR}
    )
endif()

if (BUILD_HEADLESS)
  set(LLRENDER_HEADLESS_LIBRARIES
      llrenderheadless
      )
elseif (BUILD_VULKAN)
    set(LLRENDER_LIBRARIES
        gzrender
    )
else()
    set(LLRENDER_LIBRARIES
        llrender
    )
endif()

