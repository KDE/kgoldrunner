project(kgoldrunner)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
macro_optional_find_package(SndFile)
macro_log_feature(SNDFILE_FOUND "SndFile" "SndFile (libsndfile) is a C library written by Erik de Castro Lopo for reading and writing audio files." "http://www.mega-nerd.com/libsndfile/" FALSE "" "KGoldrunner needs SndFile to decode sound files. If SndFile cannot be provided on some platform or distribution, please email the KDE Games list <kde-games-devel at kde dot org>.")

# KGoldrunner sound requires access to the OpenAL and SndFile libraries.  The
# use of Phonon, as an alternative, does not give good results and is deprecated
# (2011-05-09).
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

