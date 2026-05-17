# Parcial 1 - Filtros OpenCV
### Vision por computador
### Universidad Sergio Arboleda
### Bryan Ariza y Sebastian Urrego

Aplicación Android desarrollada en **Kotlin + C++ (NDK)** que integra **OpenCV 4.12.0** para procesamiento de imágenes en tiempo real desde la cámara y desde la galería del dispositivo.

---

## Requisitos previos

- Android Studio (versión reciente)
- NDK (Side by side) — instalar desde **Tools > SDK Manager > SDK Tools**
- CMake — instalar desde **Tools > SDK Manager > SDK Tools**
- OpenCV Android SDK 4.12.0 — descargar desde [opencv.org/releases](https://opencv.org/releases/)
- Dispositivo Android con cámara o emulador con cámara virtual

---

## Configuración del proyecto

### 1. Clonar el repositorio
```bash
git clone https://github.com/SebastianUrrego/Parcial1-Filtros-openCV.git
```

### 2. Descargar OpenCV
Descarga el **OpenCV Android SDK 4.12.0** desde la página oficial y descomprímelo en tu computadora, por ejemplo en:
```
C:/Users/TuNombre/Documents/opencv-4.12.0-android-sdk/
```

### 3. Actualizar la ruta de OpenCV
Abre el archivo `app/src/main/cpp/CMakeLists.txt` y actualiza la línea 6 con la ruta donde descomprimiste OpenCV:
```cmake
set(OpenCV_DIR "C:/Users/TuNombre/Documents/opencv-4.12.0-android-sdk/OpenCV-android-sdk/sdk/native/jni")
```
> **Importante:** Usa barras diagonales `/` y asegúrate que la ruta termine en `/sdk/native/jni`

### 4. Sincronizar Gradle
Haz clic en el ícono del **Elefante 🐘** (Sync Project with Gradle Files) y espera que termine sin errores.

### 5. Ejecutar
Conecta tu celular con **depuración USB activada** o usa un emulador, y presiona **Run ▶️**.

---

## Funcionalidades

### Punto 1 — Integración OpenCV con C++
La app integra OpenCV mediante **CMakeLists.txt** y código nativo en C++ (`native-lib.cpp`), cumpliendo el requisito de usar las librerías OpenCV en Android con NDK.

### Punto 2 — Cámara en tiempo real
- Accede a la cámara del smartphone usando `JavaCamera2View` de OpenCV
- La imagen procesada se visualiza a pantalla completa
- Soporta cambio entre **cámara trasera y delantera**

### Punto 3 — Galería de fotos
- Accede a la galería del dispositivo
- Corrige automáticamente la rotación de las fotos (metadatos EXIF)
- Muestra la imagen procesada a pantalla completa con `centerCrop`

### Punto 4 — Filtros de procesamiento de imagen
Se implementaron 4 modos aplicables tanto a la cámara en vivo como a imágenes de la galería:

| # | Efecto | Descripción | Técnica OpenCV |
|---|--------|-------------|----------------|
| 0 | **Normal** | Sin filtro, imagen original | — |
| 1 | **Sketch** | Boceto de bordes invertidos en blanco y negro | `cvtColor` + `Canny` + `bitwise_not` |
| 2 | **Sepia** | Filtro de tono cálido vintage | `transform` con matriz de color 4x4 |
| 3 | **Segmentación Verde** | Resalta solo los píxeles de color verde | `cvtColor HSV` + `inRange` + máscara |

---

## Estructura del proyecto

```
app/
├── src/
│   ├── main/
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt        # Configuración de compilación C++
│   │   │   └── native-lib.cpp        # Lógica de filtros con OpenCV en C++
│   │   ├── java/
│   │   │   └── MainActivity.kt       # Actividad principal en Kotlin
│   │   └── res/
│   │       └── layout/
│   │           └── activity_main.xml # Interfaz de usuario
```

---

## Uso de la app

| Botón | Función |
|-------|---------|
| **CAM/GAL** | Alterna entre la cámara en vivo y la galería de fotos |
| **GIRAR CAM** | Cambia entre cámara trasera y delantera |
| **FILTRO** | Cicla entre los efectos: Normal → Sketch → Sepia → Verde |

---

## Filtros implementados en C++

### Sketch (Efecto Boceto)
Convierte la imagen a escala de grises, detecta bordes con el algoritmo de Canny e invierte los colores para lograr un efecto de dibujo a lápiz.

### Sepia (Filtro Cálido)
Aplica una matriz de transformación de color 4x4 que mezcla los canales RGB para producir tonos cálidos marrones y dorados característicos de las fotos antiguas.

### Segmentación de Color Verde
Convierte la imagen al espacio de color HSV, aplica una máscara con `inRange` para aislar solo los píxeles dentro del rango de verde, y muestra únicamente esos píxeles sobre fondo negro.

---

## Dependencias

```kotlin
implementation("androidx.exifinterface:exifinterface:1.3.7")
```
- OpenCV Android SDK 4.12.0
- Android NDK (C++)
- CMake 3.22.1+

---

## Autores
**Bryan Ariza & Sebastian Urrego**
Computer Vision - IELC 5818
Universidad Sergio Arboleda
