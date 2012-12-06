CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ug-openlock-setting C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")
SET(VERSION 0.1.0)

SET(SRCS
	./src/openlock-setting.c
	./src/openlock-setting-util.c
	./src/openlock-setting-main.c
	./src/openlock-setting-pw.c
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs_lock_settings REQUIRED appcore-efl aul elementary ui-gadget-1 evas ail capi-appfw-application)

FOREACH(flag ${pkgs_lock_settings_CFLAGS})
	SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} -fvisibility=hidden -Wall")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXXFLAGS}")
SET(CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS})

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES SOVERSION 0.1.0)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_lock_settings_LDFLAGS})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION /usr/ug/lib)
#INSTALL(TARGETS "${CMAKE_CURRENT_SOURCE_DIR}/00_winset_control_top_arrow.png" DESTINATION /opt/ug/res/images/ug-openlock-setting)

