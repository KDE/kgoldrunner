project(kgoldrunner)

# macro_optional_find_package(OpenAL QUIET)
find_package(OpenAL REQUIRED)
macro_log_feature(OPENAL_FOUND "OpenAL" "OpenAL (Open Audio Library) is a free software cross-platform audio API." "http://connect.creativelabs.com/openal" FALSE "" "KGoldrunner uses OpenAL for sound output, otherwise it is possible but undesirable to build it without sound.")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
# macro_optional_find_package(SndFile )
find_package(SndFile REQUIRED)
macro_log_feature(SNDFILE_FOUND "SndFile" "libsndfile is a C library written by Erik de Castro Lopo for reading and writing audio files." "http://www.mega-nerd.com/libsndfile/" FALSE "" "SndFile is needed to decode the sound files.")

# KGoldrunner sound requires access to the OpenAL and SndFile libraries.  If
# they are not available, sound can be permanently disabled. The use of Phonon,
# as an alternative, does not give good results and is deprecated (2011-05-09).
if(OPENAL_FOUND AND SNDFILE_FOUND)
    include_directories(${OPENAL_INCLUDE_DIR} ${SNDFILE_INCLUDE_DIRS})
    include_directories(src/TagaroAudio)
    set(KGOLDRUNNER_OPENAL TRUE)
    ADD_DEFINITIONS(-DKGOLDRUNNER_USE_OPENAL)
    ADD_DEFINITIONS(-DENABLE_SOUND_SUPPORT)
endif(OPENAL_FOUND AND SNDFILE_FOUND)

add_subdirectory( src ) 
add_subdirectory( gamedata ) 
add_subdirectory( themes ) 

