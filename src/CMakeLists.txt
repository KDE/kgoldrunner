
########### next target ###############

set(kgoldrunner_SRCS
   kgrview.cpp
   kgrscene.cpp
   kgrsprite.cpp
   kgoldrunner.cpp 
   kgrthemetypes.cpp
   kgrrenderer.cpp
   kgrgame.cpp 
   kgrlevelplayer.cpp
   kgrlevelgrid.cpp
   kgrrunner.cpp
   kgrrulebook.cpp
   kgrgameio.cpp 
   kgrtimer.cpp
   kgreditor.cpp
   main.cpp 
   kgrselector.cpp
   kgrsounds.cpp 
   kgrdialog.cpp
)

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX KGOLDRUNNER VERSION_HEADER kgoldrunner_version.h)

ecm_qt_declare_logging_category(kgoldrunner_SRCS HEADER kgoldrunner_debug.h IDENTIFIER KGOLDRUNNER_LOG CATEGORY_NAME log_kgoldrunner)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*-apps-kgoldrunner.png")
ecm_add_app_icon(kgoldrunner_SRCS ICONS ${ICONS_SRCS})
add_executable(kgoldrunner ${kgoldrunner_SRCS})

target_link_libraries(kgoldrunner
    KF5KDEGames
    KF5KDEGamesPrivate
    KF5::ConfigWidgets
    KF5::DBusAddons
    KF5::KIOCore
    KF5::Crash
    KF5::WidgetsAddons
    KF5::XmlGui
)

install(TARGETS kgoldrunner  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

########### install files ###############

install(PROGRAMS org.kde.kgoldrunner.desktop  DESTINATION  ${KDE_INSTALL_APPDIR})
install(FILES kgoldrunnerui.rc  DESTINATION  ${KDE_INSTALL_KXMLGUI5DIR}/kgoldrunner)
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

