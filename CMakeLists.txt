cmake_minimum_required(VERSION 3.22)

project(
	SyncFiles
	VERSION 1.1.0
	LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)


set(CMAKE_CXX_STANDARD_REQUIRED ON)
if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()



find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(frozen REQUIRED CONFIG)
find_package(magic_enum CONFIG REQUIRED)
find_package(zydis CONFIG REQUIRED)
find_package(infoware REQUIRED CONFIG)

find_package(Catch2 CONFIG REQUIRED)
find_package(Boost_unordered CONFIG REQUIRED)
find_package(Boost_stacktrace_windbg CONFIG REQUIRED)



include(FindPackageHandleStandardArgs)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID};${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "Clang;MSVC")

    set(VS_PATH ${CMAKE_GENERATOR_INSTANCE})
    get_filename_component(VS_DIA_INC_PATH "${VS_PATH}/DIA SDK/include" ABSOLUTE CACHE)

    find_path(DIASDK_INCLUDE_DIR # Set variable DIASDK_INCLUDE_DIR
            dia2.h # Find a path with dia2.h
            HINTS "${VS_DIA_INC_PATH}"
            HINTS "${VSWHERE_LATEST}/DIA SDK/include"
            DOC "path to DIA SDK header files"
    )
    include_directories(${DIASDK_INCLUDE_DIR})
    # VS 2017
    if(CMAKE_GENERATOR MATCHES "Visual Studio 15 2017.*")
            if(CMAKE_GENERATOR MATCHES "Visual Studio.*Win64")
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib/amd64)
            elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM")
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib/arm)
            else(CMAKE_GENERATOR MATCHES "Visual Studio.*Win64")
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib)
            endif(CMAKE_GENERATOR MATCHES "Visual Studio.*Win64")
    else(CMAKE_GENERATOR MATCHES "Visual Studio 15 2017.*")
            if((CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR("${DXC_BUILD_ARCH}" STREQUAL "x64"))
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib/amd64)
            elseif((CMAKE_GENERATOR_PLATFORM STREQUAL "ARM") OR("${DXC_BUILD_ARCH}" STREQUAL "ARM"))
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib/arm)
            elseif((CMAKE_GENERATOR_PLATFORM MATCHES "ARM64.*") OR("${DXC_BUILD_ARCH}" MATCHES "ARM64.*"))
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib/arm64)
            else((CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR("${DXC_BUILD_ARCH}" STREQUAL "x64"))
                    find_library(DIASDK_GUIDS_LIBRARY NAMES diaguids.lib HINTS ${DIASDK_INCLUDE_DIR}/../lib)
            endif((CMAKE_GENERATOR_PLATFORM STREQUAL "x64") OR("${DXC_BUILD_ARCH}" STREQUAL "x64"))
    endif(CMAKE_GENERATOR MATCHES "Visual Studio 15 2017.*")

    set(DIASDK_LIBRARIES ${DIASDK_GUIDS_LIBRARY})
    set(DIASDK_INCLUDE_DIRS ${DIASDK_INCLUDE_DIR})
    set(DIA_DLL "${DIASDK_INCLUDE_DIRS}/../bin/amd64/msdia140.dll")
    # handle the QUIETLY and REQUIRED arguments and set DIASDK_FOUND to TRUE
    # if all listed variables are TRUE
    find_package_handle_standard_args(DiaSDK DEFAULT_MSG
            DIASDK_LIBRARIES DIASDK_INCLUDE_DIR)
    mark_as_advanced(DIASDK_INCLUDE_DIRS DIASDK_LIBRARIES)

endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC" OR "${CMAKE_CXX_COMPILER_ID};${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}" STREQUAL "Clang;MSVC")
        message(STATUS "building CrashHandler")
        add_definitions(-D_CRT_SECURE_NO_WARNINGS)
        include_directories("CrashHandler")

        configure_file(
                "cmake/version.rc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
                @ONLY
        )

        set(SOURCE_FILES_CRASHHANDLER
                "CrashHandler/CrashHandler.cpp"
                "CrashHandler/PCH.cpp"
                "CrashHandler/Introspection/Introspection.cpp"
                "CrashHandler/Modules/ModuleHandler.cpp"
                "CrashHandler/PDB/PDBHandler.cpp"
        )

        add_library(
                "CrashHandler"
                STATIC
                ${SOURCE_FILES_CRASHHANDLER}
                "${VERSION_HEADER}"
                "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
                ".clang-format"
                ".editorconfig"
        )

        if(DIASDK_LIBRARIES)
                        add_custom_command(TARGET "CrashHandler" POST_BUILD
                                COMMAND ${CMAKE_COMMAND} -E copy ${DIA_DLL} "./")
                target_include_directories("CrashHandler"
                        PUBLIC
                        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                        ${DIASDK_INCLUDE_DIRS}
                        ${DIASDK_INCLUDE_DIRS}/../lib
                )
                target_link_libraries("CrashHandler"
                        PUBLIC
                        ${DIASDK_INCLUDE_DIRS}/../lib/amd64/diaguids.lib
                )
        endif()

        target_link_libraries(
                "CrashHandler"
                PRIVATE
                fmt::fmt
        Boost::stacktrace_windbg
        Dbghelp.lib
        frozen::frozen
        infoware
        magic_enum::magic_enum
        Zydis::Zydis
        )
else()
        message(STATUS "not building CrashHandler ${CMAKE_CXX_COMPILER_ID}")
endif()


list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(src)
include(cmake/packaging.cmake)
