CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(starter C)

SET(VENDOR "samsung")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "com.${VENDOR}.${PACKAGE}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(BINDIR "${PREFIX}/bin")
SET(DATADIR "${PREFIX}/share")
SET(RESDIR "${PREFIX}/res")
SET(LOCALEDIR "${PREFIX}/share/locale")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

IF("${STARTER_FEATURE_LITE}" STREQUAL "ENABLE")
MESSAGE("######################################## LITE")
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	ail
	aul
	capi-system-media-key
	db-util
	dlog
	ecore
	ecore-evas
	ecore-input
	ecore-x
	edbus
	eet
	eina
	elementary
	evas
	syspopup-caller
	utilX
	vconf
	x11
	xcomposite
	xext
	appcore-efl
	glib-2.0
	ui-gadget-1
	bundle
	capi-appfw-application
	capi-appfw-app-manager
	capi-network-bluetooth
	feedback
	alarm-service
	pkgmgr-info
	deviced
	edbus
	dbus-1
	dbus-glib-1
	tts
	capi-message-port
)
ELSE()
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	ail
	aul
	capi-system-media-key
	db-util
	dlog
	ecore
	ecore-evas
	ecore-input
	ecore-x
	edbus
	eet
	eina
	elementary
	evas
	syspopup-caller
	utilX
	vconf
	x11
	xcomposite
	xext
	appcore-efl
	glib-2.0
	ui-gadget-1
	bundle
	capi-appfw-application
	capi-appfw-app-manager
	capi-network-bluetooth
	feedback
	alarm-service
	pkgmgr-info
	deviced
	edbus
	dbus-1
	dbus-glib-1
)
ENDIF()


FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wall")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")
SET(CMAKE_C_FLAGS_RELEASE "-O2")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=${PREFIX}/lib -pie")

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")
IF("${ARCH}" STREQUAL "arm")
	ADD_DEFINITIONS("-DTARGET")
	MESSAGE("add -DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DVENDOR=\"${VENDOR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
ADD_DEFINITIONS("-DPACKAGE_NAME=\"${PKGNAME}\"")
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-D_GNU_SOURCE")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

IF("${STARTER_FEATURE_LITE}" STREQUAL "ENABLE")
MESSAGE("######################################## LITE")
ADD_EXECUTABLE(${PROJECT_NAME}
	src/hw_key.c
	src/pkg_event.c
	src/menu_daemon.c
	src/starter.c
	src/x11.c
	src/lock-daemon-lite.c
	src/lockd-debug.c
	src/lockd-process-mgr.c
	src/lockd-window-mgr.c
	src/lockd-bt.c
	src/direct-access.c
	src/dbus-util.c
)
ELSE()
ADD_EXECUTABLE(${PROJECT_NAME}
	src/starter_w.c
	src/hw_key_w.c
	src/hourly_alert.c
	src/dbus-util_w.c
	src/clock-mgr.c
)
ENDIF()
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})

INSTALL(FILES ${CMAKE_SOURCE_DIR}/rd4starter DESTINATION /etc/init.d
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/rd3starter DESTINATION /etc/init.d
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
#INSTALL to /etc/init.d

ADD_SUBDIRECTORY(po)
