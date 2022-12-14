# -*- cmake -*-

project(llkdu)

# Visual Studio 2005 has a dumb bug that causes it to fail compilation
# of KDU if building with both optimisation and /WS (treat warnings as
# errors), even when the specific warnings that make it croak are
# disabled.

#set(VS_DISABLE_FATAL_WARNINGS ON)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLKDU)
include(LLMath)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${KDU_INCLUDE_DIR}
    ${LLKDU_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    )

set(llkdu_SOURCE_FILES
    llimagej2ckdu.cpp
    llkdumem.cpp
    )

set(llkdu_HEADER_FILES
    CMakeLists.txt
    
    llimagej2ckdu.h
    llkdumem.h
    )

set_source_files_properties(${llkdu_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llkdu_SOURCE_FILES ${llkdu_HEADER_FILES})

# Our KDU package is built with KDU_X86_INTRINSICS in its .vcxproj file.
# Unless that macro is also set for every consumer build, KDU freaks out,
# spamming the viewer log with alignment FUD.
set_source_files_properties(${llkdu_SOURCE_FILES}
                            PROPERTIES
                            COMPILE_DEFINITIONS
                            "KDU_X86_INTRINSICS")

if (USE_KDU)
  add_library (llkdu ${llkdu_SOURCE_FILES})

  target_link_libraries(llkdu
    ${KDU_LIBRARY}
    llcommon)

  target_compile_definitions(llkdu PRIVATE -DKDU_X86_INTRINSICS -DKDU_NO_THREADS -DKDU_NO_AVX -DKDU_NO_AVX2)

  if(WINDOWS)
    target_compile_definitions(llkdu PRIVATE -DCORESYS_IMPORTS)
  endif()
  
  # Add tests
  if (LL_TESTS)
    include(LLAddBuildTest)
    include(Tut)
    SET(llkdu_TEST_SOURCE_FILES
      llimagej2ckdu.cpp
      )
    SET(llkdu_test_additional_HEADER_FILES
      llimagej2ckdu.h
      llkdumem.h
      lltut.h
      )
    SET(llkdu_test_additional_INCLUDE_DIRS
      ${KDU_INCLUDE_DIR}
      ${LLKDU_INCLUDE_DIRS}
    )
    LL_ADD_PROJECT_UNIT_TESTS(llkdu "${llkdu_TEST_SOURCE_FILES}")
  endif (LL_TESTS)

endif (USE_KDU)
