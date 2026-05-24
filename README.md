# App Vision por Computador
### Vision por Computador
### Universidad Sergio Arboleda  
### Bryan Ariza y Sebastian Urrego

Aplicación Android desarrollada en **Kotlin + C++ (NDK)** que integra **OpenCV 4.12.0** para procesamiento de imágenes en tiempo real desde la cámara y desde la galería del dispositivo.

El proyecto fue desarrollado como solución al **Examen Práctico del Tercer Corte** de la materia de Visión por Computador.

---

# Objetivos del parcial

El examen práctico solicitaba desarrollar dos funcionalidades principales:

1. Crear una aplicación Android que detecte rostros usando el modelo **Haar Features - Viola Jones** desde la cámara frontal del smartphone.
2. Crear una aplicación Android usando OpenCV que identifique la cantidad de dinero en pesos colombianos presente en una imagen.

---

# Tecnologías utilizadas

- Kotlin
- C++ (Android NDK)
- OpenCV 4.12.0
- CMake
- Android Studio
- Haar Cascade Classifier
- Viola-Jones Face Detection
- OpenCV JavaCamera2View

---

# Requisitos previos

- Android Studio (versión reciente)
- Android SDK
- Android NDK (Side by side)
- CMake 3.22.1+
- OpenCV Android SDK 4.12.0
- Celular Android con cámara o emulador compatible

---

# Instalación

## 1. Clonar el repositorio

```bash
git clone https://github.com/SebastianUrrego/Parcial1-Filtros-openCV.git
```

---

## 2. Descargar OpenCV

Descargar el SDK oficial de OpenCV Android:

https://opencv.org/releases/

Descomprimirlo en una ruta como:

```text
C:/Users/TuNombre/Documents/opencv-4.12.0-android-sdk/
```

---

## 3. Configurar OpenCV en CMake

Abrir:

```text
app/src/main/cpp/CMakeLists.txt
```

Modificar la ruta:

```cmake
set(OpenCV_DIR "C:/Users/TuNombre/Documents/opencv-4.12.0-android-sdk/OpenCV-android-sdk/sdk/native/jni")
```

> Importante: usar `/` y no `\`

---

## 4. Sincronizar Gradle

Presionar:

```text
Sync Project with Gradle Files
```

y esperar a que Android Studio termine la sincronización.

---

## 5. Ejecutar la aplicación

- Activar depuración USB en el dispositivo
- Conectar el celular
- Presionar:

```text
Run ▶️
```

---

# Funcionalidades implementadas

# Punto 1 — Detección Facial con Viola-Jones

Se implementó un sistema de detección facial en tiempo real usando:

- Haar Cascade Classifier
- Haar Features
- Modelo Viola-Jones
- Cámara frontal del smartphone
- OpenCV CascadeClassifier

La detección funciona en tiempo real utilizando la cámara del dispositivo Android.

## Características

- Detección facial en vivo
- Soporte para cámara frontal y trasera
- Procesamiento en tiempo real
- Integración OpenCV + NDK
- Renderizado a pantalla completa

---

# Punto 2 — Detección y Clasificación de Monedas Colombianas

La aplicación cuenta con un algoritmo avanzado implementado en C++ (NDK) para detectar, medir y clasificar monedas colombianas en tiempo real utilizando procesamiento digital de imágenes con OpenCV.

## Funcionalidades y Flujo de Procesamiento

1. **Detección de Círculos:** Uso de `HoughCircles` con parámetros ajustados para evitar falsos positivos y filtrado Non-Maximum Suppression (NMS) para descartar superposiciones.
2. **Análisis de Color (HSV):** Extracción del color en el centro exacto de la moneda para determinar si es **dorada** (bronce/latón) o **plateada** (alpaca/acero). Es robusto ante reflejos de iluminación amarilla del ambiente.
3. **Monedas Bimetálicas:** Detecta contraste entre el anillo exterior y el centro para encontrar directamente las monedas de $500 y $1000.
4. **Escala Relativa:** Utiliza las monedas de $500 y $1000 como "anclas" físicas (ya que tienen el mismo tamaño en familias nuevas y viejas) para calcular la escala exacta de píxeles a milímetros (`px_to_mm`).
5. **Clasificación por Diámetro Real:**
   - **Rama Dorada:** `$100 antigua (23.0 mm)`, `$100 nueva (20.3 mm)`, `$50 antigua (20.0 mm)`.
   - **Rama Plateada:** `$200 antigua (24.4 mm)`, `$200 nueva (22.4 mm)`, `$50 nueva (17.0 mm)`.
---

# Procesamiento de imágenes

La aplicación incluye varios filtros implementados en C++ usando OpenCV.

| # | Efecto | Descripción | Técnica OpenCV |
|---|---|---|---|
| 0 | Normal | Imagen original | — |
| 1 | Sketch | Boceto blanco y negro | `Canny + bitwise_not` |
| 2 | Sepia | Efecto cálido vintage | `transform()` |
| 3 | Segmentación Verde | Detecta color verde | `HSV + inRange()` |
| 4 | Detección de Rostros | Detección facial Viola-Jones | `detectMultiScale()` |
| 5 | Detector de Monedas | Clasifica monedas y suma total | `HoughCircles + HSV` |

---

# Filtros implementados

## Sketch

Convierte la imagen a escala de grises y detecta bordes usando el algoritmo de Canny.

```cpp
cvtColor(src, gray, COLOR_BGR2GRAY);
Canny(gray, edges, 80, 150);
bitwise_not(edges, dst);
```

---

## Sepia

Aplica una transformación de color usando una matriz 4x4.

```cpp
transform(src, dst, sepiaKernel);
```

---

## Segmentación Verde

Convierte la imagen al espacio HSV y aplica una máscara.

```cpp
cvtColor(src, hsv, COLOR_BGR2HSV);
inRange(hsv, lowerGreen, upperGreen, mask);
```

---

# Estructura del proyecto

```text
app/
├── src/
│   ├── main/
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   └── native-lib.cpp
│   │   ├── java/
│   │   │   └── MainActivity.kt
│   │   └── res/
│   │       └── layout/
│   │           └── activity_main.xml
```

---

# Uso de la aplicación

| Botón | Función |
|---|---|
| CAM/GAL | Alterna entre cámara y galería |
| GIRAR CAM | Cambia entre cámara frontal y trasera |
| FILTRO | Cambia entre filtros |

---

# Dependencias

```kotlin
implementation("androidx.exifinterface:exifinterface:1.3.7")
```

Además:

- OpenCV Android SDK 4.12.0
- Android NDK
- CMake

---

# Conceptos utilizados

## Haar Features

Método basado en diferencias de intensidad entre regiones claras y oscuras.

## Viola-Jones

Algoritmo de detección de objetos en tiempo real basado en:

- Haar Features
- Imagen Integral
- AdaBoost
- Cascade Classifier

## OpenCV

Biblioteca de visión por computador usada para:

- Procesamiento de imágenes
- Detección facial
- Segmentación
- Conversión de color
- Captura de cámara

---

# Resultados

- Integración exitosa de OpenCV con Android NDK
- Procesamiento en tiempo real
- Detección facial funcional
- Aplicación estable en Android
- Uso de C++ para acelerar procesamiento

---

# Autores

**Bryan Ariza & Sebastian Urrego**  
Computer Vision - IELC 5818  
Universidad Sergio Arboleda
