# -*- cmake -*-

include(00-Common)

project(cmake)

set(cmake_SOURCE_FILES
    CMakeLists.txt

    00-Common.cmake
    APR.cmake
    Audio.cmake
    Boost.cmake
    BuildVersion.cmake
    CEFPlugin.cmake
    CEFPlugin.cmake
    CMakeCopyIfDifferent.cmake
    CURL.cmake
    Copy3rdPartyLibs.cmake
    DBusGlib.cmake
    DeploySharedLibs.cmake
    DragDrop.cmake
    Epoxy.cmake
    EXPAT.cmake
    FindAPR.cmake
    FindAutobuild.cmake
    FindFMODSTUDIO.cmake
    FindHUNSPELL.cmake
    FindNDOF.cmake
    FindOpenJPEG.cmake
    FindURIPARSER.cmake
    FindXmlRpcEpi.cmake
    FMODSTUDIO.cmake
    FreeType.cmake
    GLEXT.cmake
    GLOD.cmake
    GStreamer010Plugin.cmake
    Hunspell.cmake
    JPEG.cmake
    LLAddBuildTest.cmake
    LLAppearance.cmake
    LLAudio.cmake
    LLCharacter.cmake
    LLCommon.cmake
    LLImage.cmake
    LLImageJ2COJ.cmake
    LLInventory.cmake
    LLKDU.cmake
    LLLogin.cmake
    LLMath.cmake
    LLMessage.cmake
    LLPhysicsExtensions.cmake
    LLPlugin.cmake
    LLPrimitive.cmake
    LLRender.cmake
    LLSharedLibs.cmake
    LLTestCommand.cmake
    LLUI.cmake
    LLFileSystem.cmake
    LLWindow.cmake
    LLXML.cmake
    Linking.cmake
    MediaPluginBase.cmake
    NDOF.cmake
    OPENAL.cmake
    OpenGL.cmake
    OpenJPEG.cmake
    OpenSSL.cmake
    PNG.cmake
    PluginAPI.cmake
    Prebuilt.cmake
    PulseAudio.cmake
    Python.cmake
    SDL2.cmake
    Sentry.cmake
    TemplateCheck.cmake
    Tut.cmake
    UI.cmake
    UnixInstall.cmake
    URIPARSER.cmake
    WebP.cmake
    Variables.cmake
    ViewerMiscLibs.cmake
    VisualLeakDetector.cmake
    LibVLCPlugin.cmake
    XmlRpcEpi.cmake
    ZLIB.cmake
    )

source_group("Shared Rules" FILES ${cmake_SOURCE_FILES})

set(master_SOURCE_FILES
    ../CMakeLists.txt
    )

source_group("Master Rules" FILES ${master_SOURCE_FILES})

set_source_files_properties(${cmake_SOURCE_FILES} ${master_SOURCE_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(cmake
            cmake_dummy.cpp
            ${cmake_SOURCE_FILES}
            ${master_SOURCE_FILES}
            )
