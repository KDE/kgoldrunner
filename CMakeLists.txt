project(kgoldrunner)
cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.3.0")
set (KF5_MIN_VERSION "5.15.0")

if(NOT COMPILING_KDEGAMES)
	find_package(ECM 1.7.0 REQUIRED CONFIG)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

	find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets Core Gui)
	find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
			DocTools
			CoreAddons
			XmlGui
			Crash
			Solid
			DBusAddons
			I18n
			KIO
	)

	find_package(KF5KDEGames 4.9.0 REQUIRED)

	include(KDEInstallDirs)
	include(ECMInstallIcons)
	include(KDECMakeSettings)
	include(KDECompilerSettings NO_POLICY_SCOPE)
	include(FeatureSummary)
	include(ECMAddAppIcon)

	add_definitions(${QT_DEFINITIONS})
	add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

	include_directories(${KDEGAMES_INCLUDE_DIRS} ${KDE5_INCLUDES} ${QDBUS_INCLUDE_DIRS} ${QT_INCLUDES})
endif(NOT COMPILING_KDEGAMES)

add_subdirectory( src ) 
add_subdirectory( gamedata ) 
add_subdirectory( themes ) 
add_subdirectory( doc )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
