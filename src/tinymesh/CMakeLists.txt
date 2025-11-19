# ----------
# Common target names
# ----------
set(EIGEN_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/ext/eigen)
set(SPECTRA_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/ext/spectra/include)
set(TINYOBJ_LOADER_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/ext/tinyobjloader)
set(SOURCE_FILES ${SOURCE_FILES};${TINYOBJ_LOADER_INCLUDE_DIR}/tiny_obj_loader.h)
set(TINYPLY_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/ext/tinyply/source)
set(SOURCE_FILES ${SOURCE_FILES};${TINYPLY_INCLUDE_DIR}/tinyply.h)

# ----------
# Components
# ----------
set(SOURCE_FILES ${SOURCE_FILES} "tinymesh.h")
add_folder(SOURCE_FILES "core")
add_folder(SOURCE_FILES "filters")
add_folder(SOURCE_FILES "ops")
add_folder(SOURCE_FILES "remesh")
add_folder(SOURCE_FILES "restore")

add_library(${TINYMESH_LIBRARY} STATIC ${SOURCE_FILES})

source_group("Source Files" FILES ${SOURCE_FILES})

target_include_directories(
    ${TINYMESH_LIBRARY} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${OpenMP_INCLUDE_DIRS}
    ${EIGEN_INCLUDE_DIR}
    ${SPECTRA_INCLUDE_DIR}
    ${TINYOBJ_LOADER_INCLUDE_DIR}
    ${TINYPLY_INCLUDE_DIR})

target_link_libraries(
    ${TINYMESH_LIBRARY} PUBLIC
    ${CXX_FS_LIBRARY}
    ${OpenMP_LIBRARIES})

# Optional faster allocator linking (jemalloc / tcmalloc)
option(USE_JEMALLOC "Link with jemalloc to improve allocator performance" OFF)
option(USE_TCMALLOC "Link with tcmalloc to improve allocator performance" OFF)

if (USE_JEMALLOC)
    find_library(JEMALLOC_LIB jemalloc)
    if (JEMALLOC_LIB)
        message(STATUS "Linking jemalloc: ${JEMALLOC_LIB}")
        target_link_libraries(${TINYMESH_LIBRARY} PUBLIC ${JEMALLOC_LIB})
        target_compile_definitions(${TINYMESH_LIBRARY} PUBLIC USE_JEMALLOC)
    else()
        message(WARNING "jemalloc requested but not found. Proceeding without it.")
    endif()
elseif(USE_TCMALLOC)
    find_library(TCMALLOC_LIB tcmalloc_minimal tcmalloc)
    if (TCMALLOC_LIB)
        message(STATUS "Linking tcmalloc: ${TCMALLOC_LIB}")
        target_link_libraries(${TINYMESH_LIBRARY} PUBLIC ${TCMALLOC_LIB})
        target_compile_definitions(${TINYMESH_LIBRARY} PUBLIC USE_TCMALLOC)
    else()
        message(WARNING "tcmalloc requested but not found. Proceeding without it.")
    endif()
endif()

# Disable warning for external files
set(WARNING_DISABLED_CXX_FLAGS)
if (MSVC) 
    set(WARNING_DISABLED_CXX_FLAGS "${CMAKE_CXX_FLAGS} /w")
else()
    set(WARNING_DISABLED_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
endif()

set_source_files_properties(
    ${CMAKE_CURRENT_LIST_DIR}/core/external.cpp
    PROPERTIES
    COMPILE_FLAGS ${WARNING_DISABLED_CXX_FLAGS})
    
# Enable profiler options for MSVC
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    set_property(TARGET ${TINYMESH_LIBRARY} APPEND PROPERTY LINK_FLAGS "/DEBUG /PROFILE")
endif()
