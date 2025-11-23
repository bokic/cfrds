find_program(PYTHON "python")

if (PYTHON)
    set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py")
    set(SETUP_PY    "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
    set(OUTPUT      "${CMAKE_CURRENT_BINARY_DIR}")

    configure_file(${SETUP_PY_IN} ${SETUP_PY})

    add_custom_command(OUTPUT ${OUTPUT}
                       COMMAND ${PYTHON} setup.py build_ext install --root ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_custom_target(target ALL DEPENDS ${OUTPUT})

    install(CODE "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install)")
endif()
