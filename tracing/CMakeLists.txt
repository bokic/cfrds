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

include(FetchContent)
FetchContent_Declare(
    perfetto
    GIT_REPOSITORY https://github.com/google/perfetto.git
    GIT_TAG v44.0
    GIT_SHALLOW TRUE
)

# download perfetto
FetchContent_MakeAvailable(perfetto)

# Use the amalgamated SDK files from the cloned repository
add_library(perfetto SHARED ${perfetto_SOURCE_DIR}/sdk/perfetto.cc)
target_include_directories(perfetto PUBLIC 
    $<BUILD_INTERFACE:${perfetto_SOURCE_DIR}/sdk>
)
target_compile_definitions(perfetto PRIVATE PERFETTO_SHLIB_SDK_IMPLEMENTATION)

find_package(Threads REQUIRED)
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

install(TARGETS perfetto
    EXPORT cfrdsTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

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
