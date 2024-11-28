ecm_setup_version(${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KGOLDRUNNER
    VERSION_HEADER kgoldrunner_version.h
)

add_executable(kgoldrunner)

target_sources(kgoldrunner PRIVATE
    kgoldrunner.cpp
    kgoldrunner.h
    kgrdebug.h
    kgrdialog.cpp
    kgrdialog.h
    kgreditor.cpp
    kgreditor.h
    kgrgame.cpp
    kgrgame.h
    kgrgameio.cpp
    kgrgameio.h
    kgrglobals.h
    kgrlevelgrid.cpp
    kgrlevelgrid.h
    kgrlevelplayer.cpp
    kgrlevelplayer.h
    kgrrenderer.cpp
    kgrrenderer.h
    kgrrulebook.cpp
    kgrrulebook.h
    kgrrunner.cpp
    kgrrunner.h
    kgrscene.cpp
    kgrscene.h
    kgrselector.cpp
    kgrselector.h
    kgrsounds.cpp
    kgrsounds.h
    kgrsprite.cpp
    kgrsprite.h
    kgrthemetypes.cpp
    kgrthemetypes.h
    kgrtimer.cpp
    kgrtimer.h
    kgrview.cpp
    kgrview.h
    main.cpp


    kgoldrunner.qrc
)

ecm_qt_declare_logging_category(kgoldrunner
    HEADER kgoldrunner_debug.h
    IDENTIFIER KGOLDRUNNER_LOG
    CATEGORY_NAME kgoldrunner
    OLD_CATEGORY_NAMES log_kgoldrunner
    DESCRIPTION "KGoldrunner game"
    EXPORT KGOLDRUNNER
)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-kgoldrunner.png")
ecm_add_app_icon(kgoldrunner ICONS ${ICONS_SRCS})

target_link_libraries(kgoldrunner
    KDEGames6
    KF6::ConfigWidgets
    KF6::DBusAddons
    KF6::KIOCore
    KF6::Crash
    KF6::I18n
    KF6::WidgetsAddons
    KF6::XmlGui
    KF6::IconThemes
)

if(APPLE)
    set_target_properties(kgoldrunner PROPERTIES
           MACOSX_BUNDLE_DISPLAY_NAME "KGoldrunner"
        MACOSX_BUNDLE_BUNDLE_NAME "KGoldrunner"
        MACOSX_BUNDLE_LONG_VERSION_STRING "KGoldrunner ${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RELEASE_SERVICE_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.kde.kgoldrunner"
        MACOSX_BUNDLE_COPYRIGHT "2024 The KGoldrunner Developers")
endif()

install(TARGETS kgoldrunner  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

install(PROGRAMS org.kde.kgoldrunner.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES org.kde.kgoldrunner.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

ecm_install_icons(ICONS
    128-apps-kgoldrunner.png
    16-apps-kgoldrunner.png
    22-apps-kgoldrunner.png
    32-apps-kgoldrunner.png
    48-apps-kgoldrunner.png
    64-apps-kgoldrunner.png
    DESTINATION ${KDE_INSTALL_ICONDIR} THEME hicolor
)

ecm_qt_install_logging_categories(
    EXPORT KGOLDRUNNER
    FILE kgoldrunner.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
