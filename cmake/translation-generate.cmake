function(TRANSLATION_GENERATE QMS)
  find_package(Qt${QT_VERSION_MAJOR}LinguistTools REQUIRED)

  # Find lupdate and lrelease tools
  if (NOT Qt${QT_VERSION_MAJOR}_LUPDATE_EXECUTABLE)
    set(QT_LUPDATE "/lib/qt${QT_VERSION_MAJOR}/bin/lupdate")
    message(STATUS "NOT found lupdate, set QT_LUPDATE = ${QT_LUPDATE}")
  else()
    set(QT_LUPDATE "${Qt${QT_VERSION_MAJOR}_LUPDATE_EXECUTABLE}")
  endif()

  if (NOT Qt${QT_VERSION_MAJOR}_LRELEASE_EXECUTABLE)
    set(QT_LRELEASE "/lib/qt${QT_VERSION_MAJOR}/bin/lrelease")
    message(STATUS "NOT found lrelease, set QT_LRELEASE = ${QT_LRELEASE}")
  else()
    set(QT_LRELEASE "${Qt${QT_VERSION_MAJOR}_LRELEASE_EXECUTABLE}")
  endif()

  if(NOT ARGN)
    message(SEND_ERROR "Error: TRANSLATION_GENERATE() called without any .ts path")
    return()
  endif()

  # # 获取 translations 目录下的所有 .ts 文件
  # file(GLOB_RECURSE TS_FILES "${ARGN}/*.ts")
  # 获取指定目录下的所有 .ts 文件，不包括子目录
  file(GLOB TS_FILES "${ARGN}/*.ts")

  # Update translation files
  add_custom_target(update_translations
    COMMAND ${QT_LUPDATE} ${CMAKE_SOURCE_DIR}/src -ts ${TS_FILES}
    COMMENT "Updating translation files with lupdate"
    VERBATIM
  )

  # Generate qm files
  set(${QMS})
  foreach(TSFIL ${TS_FILES})
    get_filename_component(FIL_WE ${TSFIL} NAME_WE)
    set(QMFIL ${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.qm)
    list(APPEND ${QMS} ${QMFIL})
    add_custom_command(
      OUTPUT ${QMFIL}
      COMMAND ${QT_LRELEASE} ${TSFIL} -qm ${QMFIL}
      DEPENDS ${TSFIL} update_translations
      COMMENT "Running ${QT_LRELEASE} on ${TSFIL}"
      VERBATIM
    )
  endforeach()

  set_source_files_properties(${${QMS}} PROPERTIES GENERATED TRUE)
  set(${QMS} ${${QMS}} PARENT_SCOPE)
endfunction()
