project(kgoldrunner)

# macro_optional_find_package(OpenAL QUIET)
find_package(OpenAL REQUIRED)
macro_log_feature(OPENAL_FOUND "OpenAL" "OpenAL (Open Audio Library) is a free software cross-platform audio API." "http://connect.creativelabs.com/openal" FALSE "" "KGoldrunner needs OpenAL for sound output. If OpenAL cannot be provided on some platform or distribution, please email the author of KGoldrunner <iandw dot au at gmail dot com>.")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
# macro_optional_find_package(SndFile )
find_package(SndFile REQUIRED)
macro_log_feature(SNDFILE_FOUND "SndFile" "SndFile (libsndfile) is a C library written by Erik de Castro Lopo for reading and writing audio files." "http://www.mega-nerd.com/libsndfile/" FALSE "" "KGoldrunner needs SndFile to decode sound files. If SndFile cannot be provided on some platform or distribution, please email the author of KGoldrunner <iandw dot au at gmail dot com>.")

# KGoldrunner sound requires access to the OpenAL and SndFile libraries.  The
# use of Phonon, as an alternative, does not give good results and is deprecated
# (2011-05-09).
if(OPENAL_FOUND AND SNDFILE_FOUND)
    include_directories(${OPENAL_INCLUDE_DIR} ${SNDFILE_INCLUDE_DIRS})
    include_directories(src/TagaroAudio)
    set(KGOLDRUNNER_OPENAL TRUE)
    ADD_DEFINITIONS(-DKGOLDRUNNER_USE_OPENAL)
endif(OPENAL_FOUND AND SNDFILE_FOUND)

add_subdirectory( src ) 
add_subdirectory( gamedata ) 
add_subdirectory( themes ) 

