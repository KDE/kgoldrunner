# SPDX-FileCopyrightText: 2021-2022 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

if(WIN32)
    find_package(7z)
    set_package_properties(7z PROPERTIES
        TYPE REQUIRED
        PURPOSE "For installing SVG files as SVGZ"
    )
else()
    find_package(gzip)
    set_package_properties(gzip PROPERTIES
        TYPE REQUIRED
        PURPOSE "For installing SVG files as SVGZ"
    )
endif()


function(generate_svgz svg_file svgz_file target_prefix)
    if (NOT IS_ABSOLUTE ${svg_file})
        set(svg_file "${CMAKE_CURRENT_SOURCE_DIR}/${svg_file}")
    endif()
    if (NOT EXISTS ${svg_file})
        message(FATAL_ERROR "No such file found: ${svg_file}")
    endif()
    get_filename_component(_fileName "${svg_file}" NAME)
    get_filename_component(_svgzdir "${svgz_file}" DIRECTORY)
    if(WIN32)
        add_custom_command(
            OUTPUT ${svgz_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_svgzdir} # ensure output dir exists
            COMMAND 7z::7z
            ARGS
                a
                -tgzip
                ${svgz_file} ${svg_file}
            DEPENDS ${svg_file}
            COMMENT "Gzipping ${_fileName}"
        )
    else()
        add_custom_command(
            OUTPUT ${svgz_file}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_svgzdir} # ensure output dir exists
            COMMAND ${gzip_EXECUTABLE}
            ARGS
                -9n
                -c
                ${svg_file} > ${svgz_file}
            DEPENDS ${svg_file}
            COMMENT "Gzipping ${_fileName}"
        )
    endif()

    add_custom_target("${target_prefix}${_fileName}z" ALL DEPENDS ${svgz_file})
endfunction()
