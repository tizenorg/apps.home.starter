CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(lock-daemon C)

SET(PREFIX "/usr")
SET(EXEC_PREFIX "${PREFIX}/bin")
SET(LIBDIR "${PREFIX}/lib")
SET(CONFDIR "/etc")
SET(VERSION 0.1.0)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs_lock_daemon REQUIRED
	elementary
	appcore-efl
	heynoti
	aul
	vconf
	ecore
	evas
	ecore-evas
	eet
	x11
	dlog
	ecore-x
	utilX
	ecore-input
	glib-2.0
	ui-gadget
	bundle
)
FOREACH (flag ${pkgs_lock_daemon_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

ADD_DEFINITIONS(${EXTRA_CFLAGS})
ADD_LIBRARY(${PROJECT_NAME} SHARED
	src/lock-daemon.c
	src/lockd-debug.c
	src/lockd-process-mgr.c
)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_lock_daemon_LDFLAGS})
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION lib)

# End of a file
