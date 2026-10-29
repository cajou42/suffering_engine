set(NAME "sufferin_engine_core")

add_subdirectory(E:/glm-1.0.3/glm build)

include_directories(SYSTEM $ENV{VULKAN_SDK}/include)
include_directories(${core_external_path}/tinyobjloader-release)
include_directories(${core_external_path}/KTX-Software-main/lib/include)
include_directories(${core_external_path}/KTX-Software-main/other_include)
include_directories(${core_external_path}/KTX-Software-main/external/dfdutils)
include_directories(${core_external_path}/KTX-Software-main/utils)

set(KTX_DIR external/KTX-Software-main)
set(KTX_SOURCES
    ${KTX_DIR}/lib/src/texture.c
    ${KTX_DIR}/lib/src/hashlist.c
    ${KTX_DIR}/lib/src/checkheader.c
    ${KTX_DIR}/lib/src/swap.c
    ${KTX_DIR}/lib/src/memstream.c
    ${KTX_DIR}/lib/src/filestream.c
    ${KTX_DIR}/lib/src/vkloader.c)
add_library(ktx STATIC ${KTX_SOURCES})

if (TARGET Vulkan::Vulkan)
    target_link_libraries(ktx PUBLIC Vulkan::Vulkan)
elseif (APPLE)
    target_link_libraries(ktx PUBLIC $ENV{VULKAN_SDK}/lib/libvulkan.dylib)
elseif (NOT WIN32)
    target_link_libraries(ktx PUBLIC $ENV{VULKAN_SDK}/lib/libvulkan.so)
endif()

find_library(SDL_LIBRARY NAMES SDL3 HINTS "$ENV{VULKAN_SDK}/Lib" REQUIRED)
find_library(Slang_LIBRARY NAMES slang slang-compiler HINTS "$ENV{VULKAN_SDK}/Lib" REQUIRED)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
elseif(LINUX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WAYLAND_KHR")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")
if(MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

add_library(${NAME})

target_sources(${NAME}
  PRIVATE
    ${src_core_path}Application.cpp

  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS
      ${include_core_path}
    FILES
      ${include_core_path}Application.h
)

target_link_libraries(${NAME} PRIVATE glm::glm)

target_compile_definitions(${NAME} PRIVATE VK_NO_PROTOTYPES)
if (WIN32)
    target_compile_definitions(${NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(${NAME} PROPERTIES DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(${NAME} PRIVATE cxx_std_20)
target_link_libraries(${NAME} PRIVATE ${SDL_LIBRARY} ktx ${Slang_LIBRARY})
if (NOT WIN32)
    target_link_libraries(${NAME} PRIVATE glm::glm-header-only)
endif()