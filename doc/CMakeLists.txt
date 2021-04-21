# @author  Barthélémy von Haller
# @author  Sylvain Chapeland

# define doc build path
set(DOC_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc-build")

# define doc installation path
# ensures it ends with project name
if (NOT CMAKE_INSTALL_DOCDIR)
  set(CMAKE_INSTALL_DOCDIR "doc")
endif()
if (NOT "${CMAKE_INSTALL_DOCDIR}" MATCHES "${PROJECT_NAME}$")
  set(CMAKE_INSTALL_DOCDIR "${CMAKE_INSTALL_DOCDIR}/${PROJECT_NAME}")
endif()

# log doc build/install paths
message(STATUS "Documentation will be built in ${DOC_BUILD_DIR}")
message(STATUS "Documentation will be installed in ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}")

# general files directly from source
install(
  FILES
  ${CMAKE_SOURCE_DIR}/README.md
  TYPE DOC
)
install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/doc
  TYPE DOC
  FILES_MATCHING PATTERN "*.md" PATTERN "*.png"
)

# doxygen-generated files
include(FindDoxygen)
if (DOXYGEN_FOUND)
  if(NOT DOXYGEN_DOT_FOUND)
    message(WARNING "dot not found. Please install Graphviz.")
  endif(NOT DOXYGEN_DOT_FOUND)

  # configure the doxygen config file with current cmake settings
  configure_file(doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/documentation-config.doxygen @ONLY)

  # build documentation with target "doc"
  add_custom_target(
    doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/documentation-config.doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation using doxygen for ${PROJECT_NAME}"
    VERBATIM
    DEPENDS doc-dir
  )

  # doxygen fails creating multi-level output directory, make sure it exists
  add_custom_target(
    doc-dir
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DOC_BUILD_DIR}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Create the doxygen output directory"
    VERBATIM
  )

  # install generated files
  install(
    DIRECTORY ${DOC_BUILD_DIR}/html
    TYPE DOC
    OPTIONAL # because available only after "make doc"
  )
else ()
  message(WARNING "Doxygen not found")
endif (DOXYGEN_FOUND)
