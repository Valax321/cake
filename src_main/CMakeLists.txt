cmake_minimum_required(VERSION 3.0)

# set our module path and install prefix
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/..")

# grab our git info
include(GetGitRevisionDescription)
git_describe(GIT_SHA1 --match=NeVeRmAtCh --always --abbrev=8)

# set the USE_FOLDERS global property, this puts the targets into a CMakePredefinedTargets folder
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# output some status messages
message(STATUS "CMAKE_SYSTEM: " ${CMAKE_SYSTEM})
message(STATUS "CMAKE_SYSTEM_NAME: " ${CMAKE_SYSTEM_NAME})
message(STATUS "CMAKE_SYSTEM_VERSION: " ${CMAKE_SYSTEM_VERSION})
message(STATUS "CMAKE_SYSTEM_PROCESSOR: " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX})

# custom output macro
macro(q_set_output_dir name dir)
    foreach (OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIGUPPERCASE)
        set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_HOME_DIRECTORY}/../${dir})
        set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_HOME_DIRECTORY}/../${dir})
        set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIGUPPERCASE} ${CMAKE_HOME_DIRECTORY}/../${dir})
    endforeach()

    set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/../${dir})
    set_target_properties(${name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/../${dir})
    set_target_properties(${name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/../${dir})
endmacro()

# set directory names for the game and utils
set(Q_GAME_DIR baseq2)
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(Q_UTILS_DIR bin_nt)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(Q_UTILS_DIR bin_linux)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(Q_UTILS_DIR bin_mac)
endif()

#
# Options
#

option(MONOLITH "Embed game logic into main executable" OFF)
option(ONATIVE "Optimize for the host CPU" OFF)
option(WIN_UWP "Build for Microsoft Universal Windows Platform" OFF)
option(UTILS "Build utilities and tools" ON)

option(USE_CODEC_OGG "Build with ogg vorbis sound codec and libogg/libvorbis" ON)
option(USE_OPENAL "Build with OpenAL sound backend" ON)
option(USE_FREETYPE "Build with freetype font backend" ON)

set(CPU_OPTIMIZATION "-mmmx -msse -msse2" CACHE STRING "Which CPU specific optimitations should be used beside the compiler's default?")

#
# Compiler settings
#

# Windows specific settings
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if (${CMAKE_SIZEOF_VOID_P} EQUAL "8")
		set (Q_WINDOWS_ARCH "x64")
	else()
		set (Q_WINDOWS_ARCH "x86")
	endif()

	if (MSVC)
		if (CMAKE_CL_64 OR ${Q_WINDOWS_ARCH} EQUAL "x64")
			set (Q_WINDOWS_ARCH "x64")
		else ()
			set (Q_WINDOWS_ARCH "x86")
		endif ()
	else ()
		if ((MINGW AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64"))
			set (Q_WINDOWS_ARCH "x64")
		else ()
			set (Q_WINDOWS_ARCH "x86")
		endif()
	endif ()

	set(CMAKE_EXECUTABLE_SUFFIX "_${Q_WINDOWS_ARCH}${CMAKE_EXECUTABLE_SUFFIX}")
	set(CMAKE_SHARED_LIBRARY_PREFIX "")
	set(CMAKE_SHARED_LIBRARY_SUFFIX "_${Q_WINDOWS_ARCH}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

# compiler settings
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	# GCC and Clang settings
	add_definitions(-pipe)
	add_definitions(-Wall)
	if (CPU_OPTIMIZATION)
		add_definitions(${CPU_OPTIMIZATION})
	endif()

	# If we're building with gcc for i386 let's define -ffloat-store.
	# This helps the old and crappy x87 FPU to produce correct values.
	# Would be nice if Clang had something comparable.
	if ("${ARCH}" STREQUAL "i386")
		if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffloat-store")
		endif()
	endif()

	# -MMD to generate header dependencies. (They cannot be
	#  generated if building universal binaries on OSX)
	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MMD")
	endif()

	# the warnings are used for every profile anyway, so put them in a variable
	set(my_warn_flags "-Wno-pragmas -Wno-unused-variable -Wno-switch -Wno-unused-value -Winvalid-pch -Wno-multichar")

	if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		# append clang-specific settings for warnings (the second one make sure clang doesn't complain
		# about unknown -W flags, like -Wno-unused-but-set-variable)
		set(my_warn_flags "${my_warn_flags} -Wno-local-type-template-args -Wno-unknown-warning-option -Wno-inline-new-delete -Wno-switch-enum")
	endif()

	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		add_definitions(-march=native)
	endif()

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -O0 -ggdb -fno-strict-aliasing -fomit-frame-pointer -fwrapv ${my_warn_flags}")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -fomit-frame-pointer -fno-strict-aliasing -fomit-frame-pointer -fwrapv ${my_warn_flags}")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O2 -fomit-frame-pointer -fno-strict-aliasing -fomit-frame-pointer -fwrapv ${my_warn_flags}")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -fomit-frame-pointer -fno-strict-aliasing -fomit-frame-pointer -fwrapv ${my_warn_flags}")

	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

	if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set(CMAKE_EXECUTABLE_SUFFIX ".${CMAKE_SYSTEM_PROCESSOR}${CMAKE_EXECUTABLE_SUFFIX}")
		set(CMAKE_SHARED_LIBRARY_SUFFIX "_${CMAKE_SYSTEM_PROCESSOR}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set(CMAKE_SHARED_LIBRARY_SUFFIX "_mac${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()
elseif(MSVC)
	# MSVC settings
	
	# for MSVC 2015+, if we want UWP support
	if (WIN_UWP)
		if (NOT(MSVC_VERSION LESS 1900))
			add_definitions(-DWIN_UWP
					-DMONOLITH
					)
			
			# we have to force the WindowsApp lib instead of the usual Win32 ones
			set(CMAKE_CXX_STANDARD_LIBRARIES "WindowsApp.lib")
			set(CMAKE_C_STANDARD_LIBRARIES "WindowsApp.lib")

			# force the runtime MSVC DLLs to be used
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /ZW")
			set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD /ZW")
			set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MDd /ZW")
			set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MD /ZW")
			
			set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MDd /ZW")
			set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MD /ZW")
			set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MDd /ZW")
			set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MD /ZW")
			
			# set special UWP linker flags
			set(CMAKE_EXE_LINKER_FLAGS "/MANIFEST:NO /DEBUG:FASTLINK /WINMD")
			link_libraries(
				-nodefaultlib:vccorlib$<$<CONFIG:Debug>:d>
				-nodefaultlib:msvcrt$<$<CONFIG:Debug>:d>
				vccorlib$<$<CONFIG:Debug>:d>.lib
				msvcrt$<$<CONFIG:Debug>:d>.lib
			)
		endif()
	else()
		# define the standard link libraries
		set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib dbghelp.lib wsock32.lib ws2_32.lib rpcrt4.lib wininet.lib")
		set(CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib dbghelp.lib wsock32.lib ws2_32.lib rpcrt4.lib wininet.lib")
		
		# define standard static MSVC runtime
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MTd")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
		
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MTd")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MT")
	endif()
	
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy")

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy")
	
	# for MSVC 2013+, without /SAFESEH:NO, we can't link against older libraries
	if (NOT(MSVC_VERSION LESS 1800))
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /SAFESEH:NO")
	endif()

	add_definitions(-DWIN32
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS)

	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "cake")
	set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD 1)
endif()

#
# add projects
#
add_subdirectory(refresh)
add_subdirectory(game)
add_subdirectory(engine)
if (UTILS)
	if (NOT(WIN_UWP))
		add_subdirectory(utils)
	endif()
endif()
