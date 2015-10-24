ADD_CUSTOM_TARGET(lock_btn.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images
				${CMAKE_CURRENT_SOURCE_DIR}/lock_btn.edc ${CMAKE_BINARY_DIR}/lock_pwd/res/edje/lock_btn.edj
				DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lock_btn.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} lock_btn.edj)
INSTALL(FILES lock_btn.edj DESTINATION /usr/share/starter/lock_pwd/res/edje)

ADD_CUSTOM_TARGET(lock_entry.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images
				${CMAKE_CURRENT_SOURCE_DIR}/lock_entry.edc ${CMAKE_BINARY_DIR}/lock_pwd/res/edje/lock_entry.edj
				DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lock_entry.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} lock_entry.edj)
INSTALL(FILES lock_entry.edj DESTINATION /usr/share/starter/lock_pwd/res/edje)

ADD_CUSTOM_TARGET(lock_pwd.edj
                COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images
				${CMAKE_CURRENT_SOURCE_DIR}/lock_pwd.edc ${CMAKE_BINARY_DIR}/lock_pwd/res/edje/lock_pwd.edj
				DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lock_pwd.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} lock_pwd.edj)
INSTALL(FILES lock_pwd.edj DESTINATION /usr/share/starter/lock_pwd/res/edje)
