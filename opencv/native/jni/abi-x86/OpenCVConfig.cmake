# ===================================================================================
#  The OpenCV CMake configuration file
#
#             ** File generated automatically, do not modify **
# ===================================================================================

SET(OpenCV_VERSION 4.12.0)
SET(OpenCV_VERSION_MAJOR  4)
SET(OpenCV_VERSION_MINOR  12)
SET(OpenCV_VERSION_PATCH  0)
SET(OpenCV_VERSION_TWEAK  0)
SET(OpenCV_VERSION_STATUS "")

include(FindPackageHandleStandardArgs)

if(NOT CMAKE_VERSION VERSION_LESS 2.8.8 AND OpenCV_FIND_COMPONENTS)
  list(APPEND _OpenCV_FPHSA_ARGS HANDLE_COMPONENTS)
  set(_OpenCV_HANDLE_COMPONENTS_MANUALLY FALSE)
else()
  set(_OpenCV_HANDLE_COMPONENTS_MANUALLY TRUE)
endif()

if(CMAKE_VERSION VERSION_LESS "2.8.3")
  get_filename_component(CMAKE_CURRENT_LIST_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
endif()

get_filename_component(OpenCV_CONFIG_PATH "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
# Adjusting to point to the project root where 'opencv' is located
get_filename_component(OpenCV_INSTALL_PATH "${OpenCV_CONFIG_PATH}/../../../../" REALPATH)

if(NOT COMMAND find_host_package)
    macro(find_host_package)
        find_package(${ARGN})
    endmacro()
endif()
if(NOT COMMAND find_host_program)
    macro(find_host_program)
        find_program(${ARGN})
    endmacro()
endif()

set(OpenCV_ANDROID_NATIVE_API_LEVEL "21")

if(OpenCV_ANDROID_NATIVE_API_LEVEL GREATER ANDROID_NATIVE_API_LEVEL)
  if(NOT OpenCV_FIND_QUIETLY)
    message(WARNING "Minimum required by OpenCV API level is android-${OpenCV_ANDROID_NATIVE_API_LEVEL}")
  endif()
  set(OpenCV_FOUND 0)
  return()
endif()

if(NOT TARGET ippicv)
  add_library(ippicv STATIC IMPORTED)
  set_target_properties(ippicv PROPERTIES
    IMPORTED_LINK_INTERFACE_LIBRARIES ""
    IMPORTED_LOCATION "${OpenCV_INSTALL_PATH}/opencv/native/3rdparty/libs/x86/libippicv.a"
  )
endif()

set(OpenCV_SHARED OFF)
set(OpenCV_USE_MANGLED_PATHS FALSE)

set(OpenCV_LIB_COMPONENTS opencv_calib3d;opencv_core;opencv_dnn;opencv_features2d;opencv_flann;opencv_gapi;opencv_highgui;opencv_imgcodecs;opencv_imgproc;opencv_ml;opencv_objdetect;opencv_photo;opencv_stitching;opencv_video;opencv_videoio;opencv_java)
set(__OpenCV_INCLUDE_DIRS "${OpenCV_INSTALL_PATH}/opencv/native/jni/include")

set(OpenCV_INCLUDE_DIRS "")
foreach(d ${__OpenCV_INCLUDE_DIRS})
  get_filename_component(__d "${d}" REALPATH)
  if(NOT EXISTS "${__d}")
    if(NOT OpenCV_FIND_QUIETLY)
      message(WARNING "OpenCV: Include directory doesn't exist: '${d}'. OpenCV installation may be broken. Skip...")
    endif()
  else()
    list(APPEND OpenCV_INCLUDE_DIRS "${__d}")
  endif()
endforeach()
unset(__d)

if(NOT TARGET opencv_core)
  include(${CMAKE_CURRENT_LIST_DIR}/OpenCVModules${OpenCV_MODULES_SUFFIX}.cmake)
endif()

if(NOT CMAKE_VERSION VERSION_LESS "2.8.11")
  foreach(__component ${OpenCV_LIB_COMPONENTS})
    if(TARGET ${__component})
      set_target_properties(
          ${__component}
          PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${OpenCV_INCLUDE_DIRS}"
      )
    endif()
  endforeach()
endif()

macro(ocv_map_imported_config target)
  if(DEFINED OPENCV_MAP_IMPORTED_CONFIG)
    get_target_property(__available_configurations ${target} IMPORTED_CONFIGURATIONS)
    foreach(remap ${OPENCV_MAP_IMPORTED_CONFIG})
      if(remap MATCHES "^(.+)=(!?)([^!]+)$")
        set(__remap_config "${CMAKE_MATCH_1}")
        set(__final_config "${CMAKE_MATCH_3}")
        string(TOUPPER "${__final_config}" __final_config_upper)
        string(TOUPPER "${__remap_config}" __remap_config_upper)
        set_target_properties(${target} PROPERTIES
            MAP_IMPORTED_CONFIG_${__remap_config} "${__final_config}"
        )
      endif()
    endforeach()
  endif()
endmacro()

if(NOT OpenCV_FIND_COMPONENTS)
  set(OpenCV_FIND_COMPONENTS ${OpenCV_LIB_COMPONENTS})
  list(REMOVE_ITEM OpenCV_FIND_COMPONENTS opencv_java)
endif()

foreach(__cvcomponent ${OpenCV_FIND_COMPONENTS})
  set (__original_cvcomponent ${__cvcomponent})
  if(NOT __cvcomponent MATCHES "^opencv_")
    set(__cvcomponent opencv_${__cvcomponent})
  endif()
  list(FIND OpenCV_LIB_COMPONENTS ${__cvcomponent} __cvcomponentIdx)
  if(__cvcomponentIdx LESS 0)
    if(_OpenCV_HANDLE_COMPONENTS_MANUALLY)
      if(NOT DEFINED OpenCV_FIND_REQUIRED_${__original_cvcomponent} OR OpenCV_FIND_REQUIRED_${__original_cvcomponent})
        message(FATAL_ERROR "${__cvcomponent} is required but was not found")
      endif()
    endif()
    set(OpenCV_${__original_cvcomponent}_FOUND FALSE)
  else()
    set(OpenCV_LIBS ${OpenCV_LIBS} "${__cvcomponent}")
    set(OpenCV_${__original_cvcomponent}_FOUND TRUE)
  endif()
  if(TARGET ${__cvcomponent})
    ocv_map_imported_config(${__cvcomponent})
  endif()
endforeach()

set(OpenCV_LIBRARIES ${OpenCV_LIBS})

find_package_handle_standard_args(OpenCV REQUIRED_VARS OpenCV_INSTALL_PATH
                                  VERSION_VAR OpenCV_VERSION ${_OpenCV_FPHSA_ARGS})
