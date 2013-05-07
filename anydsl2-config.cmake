# 
# Try to find AnyDSL2 library and include path.
# Once done this will define
#
# ANYDSL2_FOUND
# ANYDSL2_INCLUDE_DIR
# ANYDSL2_LIBRARIES
# ANYDSL2_BINARIES
#
# ANYDSL2_LIBRARY_DIR
# ANYDSL2_LIBRARY_<COMPONENT>_RELEASE
# ANYDSL2_LIBRARY_<COMPONENT>_DEBUG
#

SET(PROJ_NAME      ANYDSL2)

FIND_PATH(ANYDSL2_ROOT_DIR  anydsl2-config.cmake
	PATHS
		${ANYDSL2_DIR}
		$ENV{ANYDSL2_DIR}
		$ENV{ANYDSL2_ROOT}
)

SET(ANYDSL2_CUSTOM_CMAKE_MODULE_PATH  ${ANYDSL2_ROOT_DIR}/cmake_modules)

# INCLUDE(${ANYDSL2_CUSTOM_CMAKE_MODULE_PATH}/version.cmake)


IF (NOT ANYDSL2_FIND_QUIETLY)
	MESSAGE(STATUS "Found AnyDSL2, Version ${ANYDSL2_VERSION}")
ENDIF (NOT ANYDSL2_FIND_QUIETLY)


IF (ANYDSL2_FOUND)
	SET(ANYDSL2_FIND_QUIETLY  TRUE)
ENDIF (ANYDSL2_FOUND)


FIND_PATH(ANYDSL2_INCLUDE_DIR
	NAMES
		anydsl2/irbuilder.h
		anydsl2/world.h
	PATHS
		${ANYDSL2_ROOT_DIR}/src
		${ANYDSL2_ROOT_DIR}/include
		${ANYDSL2_ROOT_DIR}/build/include
)

FIND_PATH(ANYDSL2_LIBRARY_DIR
	NAMES
		anydsl2
		anydsl2.lib
		anydsl2.so
		anydsl2.a
		libanydsl2
		libanydsl2.so
		libanydsl2.a
		anydsl2d
		anydsl2d.lib
		anydsl2d.so
		anydsl2d.a
		libanydsl2d
		libanydsl2d.so
		libanydsl2d.a
	PATHS
		${ANYDSL2_ROOT_DIR}/lib
		${ANYDSL2_ROOT_DIR}/lib32
		${ANYDSL2_ROOT_DIR}/lib64
		${ANYDSL2_ROOT_DIR}/build
		${ANYDSL2_ROOT_DIR}/build/lib
		${ANYDSL2_ROOT_DIR}/build_x64_vs2012/lib
		/usr/lib
		/usr/lib/anydsl2
		/usr/lib/libanydsl2
		/usr/local/anydsl2
		/usr/local/libanydsl2
	PATH_SUFFIXES
		${CMAKE_CONFIGURATION_TYPES}
		${CMAKE_BUILD_TYPE}
)

# FIND_PATH(ANYDSL2_BINARY_DIR
	# NAMES
		# anydsl2
		# anydsl2.dll
		# anydsl2d
		# anydsl2d.dll
	# PATHS
		# ${ANYDSL2_ROOT_DIR}/bin
		# ${ANYDSL2_ROOT_DIR}/bin32
		# ${ANYDSL2_ROOT_DIR}/bin64
		# ${ANYDSL2_ROOT_DIR}/build
		# ${ANYDSL2_ROOT_DIR}/build/bin
	# PATH_SUFFIXES
		# ${CMAKE_CONFIGURATION_TYPES}
		# ${CMAKE_BUILD_TYPE}
# )


SET(ANYDSL2_COMPONENTS
	anydsl2
	${ANYDSL2_FIND_COMPONENTS}
)

SET(ANYDSL2_DEBUG_POSTFIX   "d")


IF (ANYDSL2_LIBRARY_DIR)

	SET(ANYDSL2_LIBRARIES "")
	# SET(ANYDSL2_BINARIES "")
	SET(ANYDSL2_COMPONENTS_FOUND "")


	FOREACH (ANYDSL2_COMP ${ANYDSL2_COMPONENTS})

		## construct correct names
		########################################################
		
		STRING(TOUPPER ${ANYDSL2_COMP} ANYDSL2_COMP_NAME_UP)
		
		SET(ANYDSL2_COMP_LIBRARY_NAME_RELEASE  ANYDSL2_LIBRARY_${ANYDSL2_COMP_NAME_UP}_RELEASE)
		SET(ANYDSL2_COMP_LIBRARY_NAME_DEBUG    ANYDSL2_LIBRARY_${ANYDSL2_COMP_NAME_UP}_DEBUG)
		# SET(ANYDSL2_COMP_BINARY_NAME_RELEASE   ANYDSL2_BINARY_${ANYDSL2_COMP_NAME_UP}_RELEASE)
		# SET(ANYDSL2_COMP_BINARY_NAME_DEBUG     ANYDSL2_BINARY_${ANYDSL2_COMP_NAME_UP}_DEBUG)


		## search for the libaries
		########################################################
		
		FIND_LIBRARY(${ANYDSL2_COMP_LIBRARY_NAME_RELEASE}
			NAMES
				${ANYDSL2_COMP}
				${ANYDSL2_COMP}.lib
				${ANYDSL2_COMP}.so
				${ANYDSL2_COMP}.a
				lib${ANYDSL2_COMP}
				lib${ANYDSL2_COMP}.so
				lib${ANYDSL2_COMP}.a
			PATHS
				${ANYDSL2_LIBRARY_DIR}
			PATH_SUFFIXES
				${CMAKE_CONFIGURATION_TYPES}
				${CMAKE_BUILD_TYPE}
			DOC "The ${ANYDSL2_COMP} library"
		)
		
		FIND_LIBRARY(${ANYDSL2_COMP_LIBRARY_NAME_DEBUG}
			NAMES
				${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}
				${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}.lib
				${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}.so
				${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}.a
				lib${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}
				lib${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}.so
				lib${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}.a
			PATHS
				${ANYDSL2_LIBRARY_DIR}
			PATH_SUFFIXES
				${CMAKE_CONFIGURATION_TYPES}
				${CMAKE_BUILD_TYPE}
			DOC "The ${ANYDSL2_COMP} debug library"
		)
		
		IF (${ANYDSL2_COMP_LIBRARY_NAME_RELEASE})
		
			IF (NOT ANYDSL2_FIND_QUIETLY)
				MESSAGE(STATUS  "    ${ANYDSL2_COMP}")
			ENDIF (NOT ANYDSL2_FIND_QUIETLY)

			IF (${ANYDSL2_COMP_LIBRARY_NAME_DEBUG})
				SET(ANYDSL2_LIBRARIES
					${ANYDSL2_LIBRARIES}
					optimized ${${ANYDSL2_COMP_LIBRARY_NAME_RELEASE}}
					debug ${${ANYDSL2_COMP_LIBRARY_NAME_DEBUG}}
				)
			ELSE (${ANYDSL2_COMP_LIBRARY_NAME_DEBUG})
				SET(ANYDSL2_LIBRARIES
					${ANYDSL2_LIBRARIES}
					${${ANYDSL2_COMP_LIBRARY_NAME_RELEASE}}
				)
			ENDIF (${ANYDSL2_COMP_LIBRARY_NAME_DEBUG})
		ENDIF (${ANYDSL2_COMP_LIBRARY_NAME_RELEASE})


		## search for the binaries (optional)
		########################################################
		
		# IF(ANYDSL2_BINARY_DIR)
		
			# FIND_FILE(${ANYDSL2_COMP_BINARY_NAME_RELEASE}
				# NAMES
					# ${ANYDSL2_COMP}
					# ${ANYDSL2_COMP}.dll
				# PATHS
					# ${ANYDSL2_BINARY_DIR}
				# PATH_SUFFIXES
					# ${CMAKE_CONFIGURATION_TYPES}
					# ${CMAKE_BUILD_TYPE}
				# DOC "The ${ANYDSL2_COMP} binary"
			# )
			
			# FIND_FILE(${ANYDSL2_COMP_BINARY_NAME_DEBUG}
				# NAMES
					# ${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}
					# ${ANYDSL2_COMP}${ANYDSL2_DEBUG_POSTFIX}.dll
				# PATHS
					# ${ANYDSL2_BINARY_DIR}
				# PATH_SUFFIXES
					# ${CMAKE_CONFIGURATION_TYPES}
					# ${CMAKE_BUILD_TYPE}
				# DOC "The ${ANYDSL2_COMP} debug binary"
			# )
			
			# IF (${ANYDSL2_COMP_BINARY_NAME_RELEASE})
				# SET(ANYDSL2_BINARIES
					# ${ANYDSL2_BINARIES}
					# ${${ANYDSL2_COMP_BINARY_NAME_RELEASE}}
				# )
			# ENDIF (${ANYDSL2_COMP_BINARY_NAME_RELEASE})
			
			# IF (${ANYDSL2_COMP_BINARY_NAME_DEBUG})
				# SET(ANYDSL2_BINARIES
					# ${ANYDSL2_BINARIES}
					# ${${ANYDSL2_COMP_BINARY_NAME_DEBUG}}
				# )
			# ENDIF (${ANYDSL2_COMP_BINARY_NAME_DEBUG})

		# ENDIF(ANYDSL2_BINARY_DIR)
		
		MARK_AS_ADVANCED(
			${ANYDSL2_COMP_LIBRARY_NAME_RELEASE}
			${ANYDSL2_COMP_LIBRARY_NAME_DEBUG}
			# ${ANYDSL2_COMP_BINARY_NAME_RELEASE}
			# ${ANYDSL2_COMP_BINARY_NAME_DEBUG}
		)
		
	ENDFOREACH (ANYDSL2_COMP ${ANYDSL2_COMPONENTS})

ENDIF (ANYDSL2_LIBRARY_DIR)

MESSAGE(STATUS "ANYDSL2_LIBRARIES: ${ANYDSL2_LIBRARIES}")
# MESSAGE(STATUS "ANYDSL2_BINARIES: ${ANYDSL2_BINARIES}")


IF (ANYDSL2_INCLUDE_DIR AND ANYDSL2_LIBRARIES)
    SET( ANYDSL2_FOUND TRUE CACHE BOOL "Set to 1 if AnyDSL2 is found, 0 otherwise" FORCE)
ELSE (ANYDSL2_INCLUDE_DIR AND ANYDSL2_LIBRARIES)
    SET( ANYDSL2_FOUND FALSE CACHE BOOL "Set to 1 if AnyDSL2 is found, 0 otherwise" FORCE)
ENDIF (ANYDSL2_INCLUDE_DIR AND ANYDSL2_LIBRARIES)


MARK_AS_ADVANCED( ANYDSL2_FOUND )