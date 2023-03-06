if(WIN32 OR CYGWIN)
   include_directories(
     ${CUSTOM_XGETOPT_DIR}
     )
else()
  include_directories(
    )
endif()

# digilent_vector_plotter
list(APPEND digilent_vector_plotter_SOURCES
    digilent_vector_plotter.cpp
  )
if(WIN32 OR CYGWIN)
    list(APPEND digilent_vector_plotter_SOURCES
      non_posix.c
    )
endif()

add_executable(digilent_vector_plotter
  ${digilent_vector_plotter_SOURCES}
  )

if(WIN32 OR CYGWIN)
  target_link_libraries(digilent_vector_plotter
    ${XGETOPT_LIBRARIES}
    )
else()
  target_link_libraries(digilent_vector_plotter
    )
endif()

if(WIN32 OR CYGWIN)
  set_target_properties(digilent_vector_plotter
    PROPERTIES COMPILE_FLAGS "-DUSE_REGEX_STATIC"
    )
  target_link_libraries(digilent_vector_plotter winmm ws2_32 setupapi)
endif()


install(TARGETS digilent_vector_plotter DESTINATION ${INSTALL_BIN_DIR})
