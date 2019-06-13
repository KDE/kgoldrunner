project(kgoldrunner)

cmake_minimum_required (VERSION 3.5 FATAL_ERROR)
set (QT_MIN_VERSION "5.7.0")
set (KF5_MIN_VERSION "5.30.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED CONFIG)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Gui Widgets)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    DocTools
    I18n
    KIO
    WidgetsAddons
    XmlGui
)

find_package(KF5KDEGames 4.9.0 REQUIRED)

include_directories(BEFORE ${PHONON_INCLUDES})

include(FeatureSummary)
include(ECMInstallIcons)
include(KDEInstallDirs)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
if (${KF5Config_VERSION} STRGREATER "5.56.0")
        add_definitions(-DQT_NO_FOREACH)
        MESSAGE(STATUS "compile without foreach")
endif()

add_subdirectory(src)
add_subdirectory(gamedata)
add_subdirectory(themes)
add_subdirectory(doc)

if (${ECM_VERSION} STRGREATER "5.58.0")
    install(FILES kgoldrunner.categories  DESTINATION  ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
else()
    install(FILES kgoldrunner.categories  DESTINATION ${KDE_INSTALL_CONFDIR})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
