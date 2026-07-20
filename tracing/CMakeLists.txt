set(ENABLE_PERFETTO OFF CACHE BOOL "Enable perfetto tracing")

# Define the function to enable tracing for a specific target
function(target_trace_enable target)
    if (ENABLE_PERFETTO)
        if(APPLE)
            target_link_libraries(${target} PRIVATE tracing)
            set_target_properties(${target} PROPERTIES ENABLE_EXPORTS ON)
        else()
            target_link_libraries(${target} PRIVATE tracing "-rdynamic")
        endif()
        target_compile_definitions(${target} PRIVATE ENABLE_PERFETTO)
        
        # Enable instrument-functions for the target source files
        target_compile_options(${target} PRIVATE "-finstrument-functions")
    endif()
endfunction()

if (NOT ENABLE_PERFETTO)
    return()
endif()

set(PERFETTO_CACHE_DIR "${CMAKE_SOURCE_DIR}/.cache/perfetto" CACHE PATH "Directory for cached Perfetto SDK")
set(PERFETTO_SRC_DIR "${PERFETTO_CACHE_DIR}/src")
set(PERFETTO_LIB_DIR "${PERFETTO_CACHE_DIR}/lib")
set(PERFETTO_CACHED_LIB "${PERFETTO_LIB_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}perfetto${CMAKE_SHARED_LIBRARY_SUFFIX}")

if(NOT EXISTS "${PERFETTO_SRC_DIR}/sdk/perfetto.cc")
    message(STATUS "Fetching Perfetto repository (one-time setup)...")
    include(FetchContent)
    FetchContent_Declare(
        perfetto
        GIT_REPOSITORY https://github.com/google/perfetto.git
        GIT_TAG v44.0
        GIT_SHALLOW TRUE
        SOURCE_DIR "${PERFETTO_SRC_DIR}"
    )
    FetchContent_MakeAvailable(perfetto)
else()
    message(STATUS "Using cached Perfetto source at ${PERFETTO_SRC_DIR}")
endif()

find_package(Threads REQUIRED)

if(EXISTS "${PERFETTO_CACHED_LIB}")
    message(STATUS "Using precompiled Perfetto library at ${PERFETTO_CACHED_LIB}")
    add_library(perfetto SHARED IMPORTED GLOBAL)
    set_target_properties(perfetto PROPERTIES
        IMPORTED_LOCATION "${PERFETTO_CACHED_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${PERFETTO_SRC_DIR}/sdk"
    )
    target_link_libraries(perfetto INTERFACE Threads::Threads ${CMAKE_DL_LIBS})

    install(FILES "${PERFETTO_CACHED_LIB}"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else()
    message(STATUS "Building Perfetto shared library for cache...")
    file(MAKE_DIRECTORY "${PERFETTO_LIB_DIR}")
    
    # Use the amalgamated SDK files from the cached repository
    add_library(perfetto SHARED ${PERFETTO_SRC_DIR}/sdk/perfetto.cc)
    target_include_directories(perfetto PUBLIC 
        $<BUILD_INTERFACE:${PERFETTO_SRC_DIR}/sdk>
    )
    target_compile_definitions(perfetto PRIVATE PERFETTO_SHLIB_SDK_IMPLEMENTATION)

    target_link_libraries(perfetto PRIVATE Threads::Threads ${CMAKE_DL_LIBS})

    # Set default symbol visibility to hidden, and enable -fPIC so the 
    # shared library can be built correctly.
    set_target_properties(perfetto PROPERTIES
        C_VISIBILITY_PRESET default
        CXX_VISIBILITY_PRESET default
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON
    )

    if(APPLE)
        set_target_properties(perfetto PROPERTIES
            INSTALL_RPATH "@loader_path"
            BUILD_WITH_INSTALL_RPATH ON
        )
    endif()

    add_custom_command(TARGET perfetto POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:perfetto>
        "${PERFETTO_CACHED_LIB}"
        COMMENT "Caching built libperfetto library to ${PERFETTO_CACHED_LIB}"
    )

    install(TARGETS perfetto
        EXPORT cfrdsTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

# Internal library for tracing hooks (now mixed C and C++)
add_library(tracing SHARED 
    tracing.c
    perfetto_backend.cpp
)
target_link_libraries(tracing PUBLIC perfetto)
target_compile_definitions(tracing PUBLIC ENABLE_PERFETTO)

# Set default symbol visibility to hidden, and enable -fPIC.
set_target_properties(tracing PROPERTIES
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
)

if(APPLE)
    set_target_properties(tracing PROPERTIES
        INSTALL_RPATH "@loader_path"
        BUILD_WITH_INSTALL_RPATH ON
    )
endif()

# Allow targets linking to 'tracing' to find the header
target_include_directories(tracing PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/tracing>
)

install(TARGETS tracing
    EXPORT cfrdsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tracing
)
