# -*- cmake -*-

include(Audio)

set(LLAUDIO_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llaudio
    )

# be exhaustive here
set(LLAUDIO_LIBRARIES llaudio ${VORBISENC_LIBRARIES} ${VORBISFILE_LIBRARIES} ${VORBIS_LIBRARIES} ${OGG_LIBRARIES} ${OPENAL_LIBRARIES})
