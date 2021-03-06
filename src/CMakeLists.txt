CMAKE_MINIMUM_REQUIRED(VERSION 2.8.1 FATAL_ERROR)

INCLUDE_DIRECTORIES ( ${PROJ_SOURCE_DIR} )
LINK_DIRECTORIES ( ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} )

IF (NOT MSVC)
    ADD_DEFINITIONS ( "-std=c++11" )
    SET ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wunused" )
ELSE ()
    ADD_DEFINITIONS ( -D_SCL_SECURE_NO_WARNINGS )
    ADD_DEFINITIONS ( -D_CRT_SECURE_NO_WARNINGS )
    SET ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /wd4800 /wd4520" )
    SET ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /wd4800 /wd4520" )
ENDIF ()

# check for wfv2
FIND_PACKAGE ( WFV2 QUIET )

IF ( WFV2_FOUND )
    MESSAGE ( STATUS "Building with WFV2." )
    ADD_DEFINITIONS ( "-DWFV2_SUPPORT" )
    INCLUDE_DIRECTORIES ( ${WFV2_INCLUDE_DIR} )
    LINK_DIRECTORIES ( ${WFV2_LIBRARY_DIR} )
ELSE ()
    MESSAGE ( STATUS "Building without WFV2. Specify WFV2_DIR to compile with WFV2." )
ENDIF ()

# check for possible llvm extension
FIND_PACKAGE ( LLVM QUIET )

IF ( LLVM_FOUND )
    MESSAGE ( STATUS "Building with LLVM ${LLVM_PACKAGE_VERSION}." )
    ADD_DEFINITIONS ( ${LLVM_DEFINITIONS} )
    ADD_DEFINITIONS ( "-DLLVM_SUPPORT" )
    INCLUDE_DIRECTORIES ( ${LLVM_INCLUDE_DIRS} )
    LINK_DIRECTORIES ( ${LLVM_LIBRARY_DIRS} )
ELSE ()
    MESSAGE ( STATUS "Building without LLVM. Specify LLVM_DIR to compile with LLVM." )
ENDIF ()

INCLUDE ( thorin-shared )

# build thorin lib
ADD_SUBDIRECTORY ( thorin )
