# SPDX-FileCopyrightText: 2022 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

set(KGOLDRUNNER_INSTALL_THEMESDIR "${KDE_INSTALL_DATADIR}/kgoldrunner/themes")

function(install_theme id)
    cmake_parse_arguments(ARG "" "" "FILES" ${ARGN})
    install( FILES ${id}.desktop DESTINATION ${KGOLDRUNNER_INSTALL_THEMESDIR} )

    foreach(_file ${ARG_FILES})
        get_filename_component(_dir "${_file}" DIRECTORY)
        get_filename_component(_extension "${_file}" EXT)

        if (_extension STREQUAL ".svg")
            set(svgz "${CMAKE_CURRENT_BINARY_DIR}/${_file}z")
            generate_svgz("${_file}" "${svgz}" "theme-${_dir}-")
            set(_file "${svgz}")
        endif()
        install( FILES ${_file} DESTINATION ${KGOLDRUNNER_INSTALL_THEMESDIR}/${_dir} )
    endforeach()
endfunction()

install_theme(default
    FILES
        default/set.svg
        default/actors.svg
        default/default.png

        default/climb.wav
        default/completed.ogg
        default/death.ogg
        default/dig.ogg
        default/falling.ogg
        default/gameover.ogg
        default/gold.ogg
        default/ladder.ogg
        default/step.wav
        default/victory.ogg
)

install_theme(black-on-white
    FILES
        accessible/black-on-white-set.svg
        accessible/black-on-white-actors.svg
        accessible/black-on-white.png
)

install_theme(egypt
    FILES
        egypt/egypt_kgr.svg
        egypt/actors.svg
        egypt/egypt.png
)

install_theme(kgr_geek
    FILES
        kgr_geek/set.svg
        kgr_geek/actors.svg
        kgr_geek/kgr_geek.png
)

install_theme(nostalgia
    FILES
        nostalgia/set.svg
        nostalgia/actors.svg
        nostalgia/nostalgia.png
)

install_theme(nostalgia-blues
    FILES
        nostalgia/blue-set.svg
        nostalgia/blue-actors.svg
        nostalgia/nostalgia-blues.png
)

install( FILES README DESTINATION ${KGOLDRUNNER_INSTALL_THEMESDIR})
install( FILES kgoldrunner.knsrc  DESTINATION  ${KDE_INSTALL_DATADIR}/knsrcfiles )
