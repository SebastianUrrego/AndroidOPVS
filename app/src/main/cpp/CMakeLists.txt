cmake_minimum_required(VERSION 3.22.1)

project("opencv_parcial")

# ── Rutas del SDK de OpenCV dentro del proyecto ─────────────────────────────
# Headers de OpenCV (opencv2/core.hpp, opencv2/imgproc.hpp, etc.)
set(OPENCV_INCLUDE "${CMAKE_SOURCE_DIR}/../../../../opencv/native/jni/include")

# Librería compartida principal de OpenCV para Android
set(OPENCV_LIB_DIR "${CMAKE_SOURCE_DIR}/../../../../opencv/native/libs/${ANDROID_ABI}")

# ── Importar libopencv_java4.so como librería precompilada ──────────────────
add_library(opencv_java4 SHARED IMPORTED)
set_target_properties(opencv_java4 PROPERTIES
        IMPORTED_LOCATION "${OPENCV_LIB_DIR}/libopencv_java4.so")

# ── Definir nuestra librería nativa ─────────────────────────────────────────
add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp)

# ── Incluir headers de OpenCV ────────────────────────────────────────────────
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${OPENCV_INCLUDE})

# ── Alineamiento de 16 KB (requerido por Android 15+ / Google Play) ──────────
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
        "-Wl,-z,max-page-size=16384")

# ── Enlazar dependencias ─────────────────────────────────────────────────────
target_link_libraries(${CMAKE_PROJECT_NAME}
        android
        log
        opencv_java4)