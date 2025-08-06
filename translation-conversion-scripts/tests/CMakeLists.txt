#####################################################################
#                                                                   #
#   IMPORTANT NOTE                                                  #
# By Default these tests will not show up in Qt Creator             #
# Either enable them in project -> testing -> check CTest checkbox  #
# or run them locally by e.g. "./test_tsTojsonConverter.py"         #
#                                                                   #
#####################################################################

add_test (NAME python-script-tests
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_tsTojsonConverter.py ${TS_FILES_STR} ${CMAKE_CURRENT_BINARY_DIR}/translations
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB TESTS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.py
)

add_custom_target(
    python-script-tests
    SOURCES ${TESTS}
)
