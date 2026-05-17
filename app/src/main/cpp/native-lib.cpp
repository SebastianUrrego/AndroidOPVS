#include <jni.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

using namespace cv;
using namespace std;

static CascadeClassifier g_faceCascade;
static long g_totalAmount = 0;

// ══════════════════════════════════════════════════════════════
// INICIALIZAR HAAR CASCADE
// ══════════════════════════════════════════════════════════════
extern "C" JNIEXPORT void JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_initFaceCascade(
        JNIEnv *env, jobject, jstring cascade_path) {
    const char* path = env->GetStringUTFChars(cascade_path, nullptr);
    g_faceCascade.load(path);
    env->ReleaseStringUTFChars(cascade_path, path);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_getDetectedAmount(
        JNIEnv*, jobject) {
    return (jlong)g_totalAmount;
}

// ══════════════════════════════════════════════════════════════
// AUXILIAR — Muestrea el color HSV mediano en una corona circular (anillo)
// ══════════════════════════════════════════════════════════════
struct HSVStats { double H, S, V; };

static HSVStats getRingHSV(const Mat& rgba, int cx, int cy, int r, float minPct, float maxPct) {
    Mat bgr, hsv;
    cvtColor(rgba, bgr, COLOR_RGBA2BGR);
    cvtColor(bgr, hsv, COLOR_BGR2HSV);

    int rMin = (int)(r * minPct);
    int rMax = (int)(r * maxPct);

    vector<double> vH, vS, vV;
    vH.reserve(400); vS.reserve(400); vV.reserve(400);

    for (int dy = -rMax; dy <= rMax; dy += 2) {
        for (int dx = -rMax; dx <= rMax; dx += 2) {
            int distSq = dx*dx + dy*dy;
            if (distSq < rMin*rMin || distSq > rMax*rMax) continue;
            int px = cx + dx, py = cy + dy;
            if (px < 0 || py < 0 || px >= rgba.cols || py >= rgba.rows) continue;
            Vec3b p = hsv.at<Vec3b>(py, px);
            vH.push_back(p[0]);
            vS.push_back(p[1]);
            vV.push_back(p[2]);
        }
    }
    if (vH.empty()) return {0, 0, 0};

    size_t mid = vH.size() / 2;
    nth_element(vH.begin(), vH.begin()+mid, vH.end());
    nth_element(vS.begin(), vS.begin()+mid, vS.end());
    nth_element(vV.begin(), vV.begin()+mid, vV.end());
    return {vH[mid], vS[mid], vV[mid]};
}

// Estructuras de datos para clasificación de monedas
enum CoinCategory {
    BIMETALLIC_1000, // Centro Plata, Borde Oro
    BIMETALLIC_500,  // Centro Oro, Borde Plata
    MONO_SILVER,     // Todo Plata ($200 vieja/nueva, $100 nueva)
    MONO_GOLD        // Todo Oro/Cobre ($100 vieja, $50 vieja/nueva)
};

struct DetectedCoin {
    int cx, cy, r;
    CoinCategory category;
    int index;
};

struct CoinInfo {
    long value;
    string label;
    Scalar drawColor;
};

// ══════════════════════════════════════════════════════════════
// FUNCIÓN PRINCIPAL
// ══════════════════════════════════════════════════════════════
extern "C" JNIEXPORT void JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_processImageNative(
        JNIEnv*, jobject, jlong mat_addr_rgba, jint effect_mode) {

    Mat& rgba = *(Mat*)mat_addr_rgba;

    // ── MODO 1: Sketch ────────────────────────────────────────────
    if (effect_mode == 1) {
        Mat gray, edges;
        cvtColor(rgba, gray, COLOR_RGBA2GRAY);
        Canny(gray, edges, 50, 150);
        bitwise_not(edges, edges);
        cvtColor(edges, rgba, COLOR_GRAY2RGBA);

    // ── MODO 2: Sepia ─────────────────────────────────────────────
    } else if (effect_mode == 2) {
        Mat sepia;
        Mat kernel = (Mat_<float>(4,4) <<
                0.272, 0.534, 0.131, 0,
                0.349, 0.686, 0.168, 0,
                0.393, 0.769, 0.189, 0,
                0,     0,     0,     1);
        transform(rgba, sepia, kernel);
        sepia.copyTo(rgba);

    // ── MODO 3: Segmentación verde ────────────────────────────────
    } else if (effect_mode == 3) {
        Mat hsv, mask, result;
        cvtColor(rgba, hsv, COLOR_RGBA2BGR);
        cvtColor(hsv, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(35, 50, 50), Scalar(85, 255, 255), mask);
        result = Mat::zeros(rgba.size(), rgba.type());
        rgba.copyTo(result, mask);
        result.copyTo(rgba);

    // ── MODO 4: DETECCIÓN DE ROSTROS (Haar Cascade / Viola-Jones) ─
    } else if (effect_mode == 4) {
        if (!g_faceCascade.empty()) {
            Mat gray;
            cvtColor(rgba, gray, COLOR_RGBA2GRAY);
            equalizeHist(gray, gray);

            vector<Rect> faces;
            g_faceCascade.detectMultiScale(
                    gray, faces,
                    1.05,
                    4,
                    CASCADE_SCALE_IMAGE,
                    Size(60, 60),
                    Size(rgba.cols / 2, rgba.rows / 2)
            );

            for (const Rect& face : faces) {
                rectangle(rgba, face, Scalar(0, 255, 0, 255), 2);
                int len = face.width / 5;
                line(rgba, face.tl(), {face.x + len, face.y}, Scalar(0,255,0,255), 4);
                line(rgba, face.tl(), {face.x, face.y + len}, Scalar(0,255,0,255), 4);
                line(rgba, {face.x+face.width, face.y}, {face.x+face.width-len, face.y}, Scalar(0,255,0,255), 4);
                line(rgba, {face.x+face.width, face.y}, {face.x+face.width, face.y+len}, Scalar(0,255,0,255), 4);
                line(rgba, {face.x, face.y+face.height}, {face.x+len, face.y+face.height}, Scalar(0,255,0,255), 4);
                line(rgba, {face.x, face.y+face.height}, {face.x, face.y+face.height-len}, Scalar(0,255,0,255), 4);
                line(rgba, face.br(), {face.x+face.width-len, face.y+face.height}, Scalar(0,255,0,255), 4);
                line(rgba, face.br(), {face.x+face.width, face.y+face.height-len}, Scalar(0,255,0,255), 4);

                string label = "Rostro";
                int baseline = 0;
                Size ts = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);
                Rect bgRect(face.x, face.y - ts.height - 8, ts.width + 8, ts.height + 8);
                if (bgRect.y >= 0)
                    rectangle(rgba, bgRect, Scalar(0, 0, 0, 180), FILLED);
                putText(rgba, label, Point(face.x + 4, face.y - 4),
                        FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0, 255), 2);
            }

            string txt = "Rostros: " + to_string(faces.size());
            putText(rgba, txt, Point(10, 38), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0,0,0,255), 4);
            putText(rgba, txt, Point(10, 38), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0, 255, 0, 255), 2);
        } else {
            putText(rgba, "Cascade no disponible", Point(10, 40),
                    FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255, 255), 2);
        }

    // ── MODO 5: DETECCIÓN DE MONEDAS COLOMBIANAS ──────────────────
    } else if (effect_mode == 5) {
        Mat gray, blurred;
        cvtColor(rgba, gray, COLOR_RGBA2GRAY);

        // Suavizado Gaussiano + Mediana optimizado para eliminar vetas de madera sin perder monedas
        GaussianBlur(gray, blurred, Size(7, 7), 1.5);
        medianBlur(blurred, blurred, 7);

        vector<Vec3f> rawCircles;
        float minDist   = (float)rgba.cols * 0.08f;
        int   minRadius = (int)(rgba.cols * 0.035f);
        int   maxRadius = (int)(rgba.cols * 0.14f);

        // Bajamos el umbral param2 de 38 a 31 para detectar todas las monedas con bordes suaves o en sombra
        HoughCircles(blurred, rawCircles, HOUGH_GRADIENT,
                     1.0,
                     minDist,
                     70,
                     31,
                     minRadius,
                     maxRadius);

        // ── FILTRADO NO-MÁXIMO (NMS) PARA EVITAR TRASLAPES ──
        vector<Vec3f> cleanCircles;
        for (const auto& c : rawCircles) {
            bool keep = true;
            for (const auto& accepted : cleanCircles) {
                float dist = sqrt(pow(c[0] - accepted[0], 2) + pow(c[1] - accepted[1], 2));
                float maxOverlapDist = (c[2] + accepted[2]) * 0.70f;
                if (dist < maxOverlapDist) {
                    keep = false;
                    break;
                }
            }
            if (keep) {
                cleanCircles.push_back(c);
            }
        }

        // Paso 1: Analizar perfil Center-vs-Edge de cada moneda
        vector<DetectedCoin> detectedCoins;
        int idx = 0;
        for (const Vec3f& c : cleanCircles) {
            int cx = cvRound(c[0]), cy = cvRound(c[1]), r = cvRound(c[2]);
            if (cx - r < 0 || cy - r < 0 || cx + r >= rgba.cols || cy + r >= rgba.rows) continue;

            // Muestreo del Centro (0% a 42% del radio)
            HSVStats centerHSV = getRingHSV(rgba, cx, cy, r, 0.0f, 0.42f);
            // Muestreo del Borde (72% a 90% del radio)
            HSVStats edgeHSV   = getRingHSV(rgba, cx, cy, r, 0.72f, 0.90f);

            // ── ANÁLISIS DE DIFERENCIA DE SATURACIÓN (INDEPENDIENTE DE LA LUZ) ──
            double centerS = centerHSV.S;
            double edgeS   = edgeHSV.S;
            double satDiff = edgeS - centerS; // Borde menos Centro

            CoinCategory cat;
            if (satDiff > 15.0) {
                // El borde es notablemente más dorado/saturado que el centro plata -> $1.000 bimetálica
                cat = BIMETALLIC_1000;
            } else if (satDiff < -15.0) {
                // El centro es notablemente más dorado/saturado que el borde plata -> $500 bimetálica
                cat = BIMETALLIC_500;
            } else {
                // Monometálicas (el color es uniforme entre centro y borde)
                double avgS = (centerS + edgeS) / 2.0;
                if (avgS > 45.0) {
                    cat = MONO_GOLD;   // Color uniforme dorado/cobre -> $100 vieja / $50
                } else {
                    cat = MONO_SILVER; // Color uniforme plateado/gris -> $200 / $100 nueva
                }
            }

            detectedCoins.push_back({cx, cy, r, cat, idx++});
        }

        // ── AUTOCALIBRACIÓN DINÁMICA DE ESCALA FÍSICA (Píxeles a Milímetros) ──
        float px_to_mm = -1.0f;

        vector<float> r_1000s;
        vector<float> r_500s;
        for (const auto& c : detectedCoins) {
            if (c.category == BIMETALLIC_1000) r_1000s.push_back(c.r);
            if (c.category == BIMETALLIC_500)  r_500s.push_back(c.r);
        }

        if (!r_1000s.empty()) {
            float sum = 0;
            for (float r : r_1000s) sum += r;
            px_to_mm = (sum / r_1000s.size()) / 13.35f; // Calibrar con la de $1.000 (radio = 13.35mm)
        } else if (!r_500s.empty()) {
            float sum = 0;
            for (float r : r_500s) sum += r;
            px_to_mm = (sum / r_500s.size()) / 11.85f; // Calibrar con la de $500 (radio = 11.85mm)
        }

        // Si no hay bimetálicas en la foto, estimamos la escala usando las monometálicas más grandes
        if (px_to_mm <= 0.0f) {
            float max_silver_r = -1.0f;
            float max_gold_r = -1.0f;
            for (const auto& c : detectedCoins) {
                if (c.category == MONO_SILVER && c.r > max_silver_r) max_silver_r = c.r;
                if (c.category == MONO_GOLD && c.r > max_gold_r)     max_gold_r = c.r;
            }

            if (max_silver_r > 0.0f) {
                px_to_mm = max_silver_r / 12.20f; // Asumimos que la plata más grande es $200 vieja (r = 12.2mm)
            } else if (max_gold_r > 0.0f) {
                px_to_mm = max_gold_r / 11.50f;   // Asumimos que el oro más grande es $100 vieja (r = 11.5mm)
            } else {
                px_to_mm = (float)rgba.cols * 0.005f; // Fallback absoluto
            }
        }

        // Paso 3: Clasificar cada moneda comparando su diámetro calculado con las dimensiones oficiales
        vector<CoinInfo> classified(detectedCoins.size(), {0, "?", Scalar(180, 180, 180, 255)});

        for (const auto& c : detectedCoins) {
            float calc_dia = (c.r * 2.0f) / px_to_mm; // Diámetro calculado de la moneda en milímetros reales

            if (c.category == BIMETALLIC_1000) {
                classified[c.index] = {1000, "$1.000", Scalar(180, 220, 255, 255)};
            } else if (c.category == BIMETALLIC_500) {
                classified[c.index] = {500, "$500", Scalar(0, 215, 255, 255)};
            } else if (c.category == MONO_SILVER) {
                // Posibles: $200 Vieja (24.4mm), $200 Nueva (22.0mm), $100 Nueva (20.3mm)
                float d_200_old = abs(calc_dia - 24.4f);
                float d_200_new = abs(calc_dia - 22.0f);
                float d_100_new = abs(calc_dia - 20.3f);

                float min_diff = min({d_200_old, d_200_new, d_100_new});
                if (min_diff == d_100_new) {
                    classified[c.index] = {100, "$100", Scalar(200, 200, 200, 255)};
                } else {
                    classified[c.index] = {200, "$200", Scalar(255, 255, 255, 255)};
                }
            } else if (c.category == MONO_GOLD) {
                // Posibles: $100 Vieja (23.0mm), $50 Vieja (21.0mm), $50 Nueva Cobre (17.0mm)
                float d_100_old = abs(calc_dia - 23.0f);
                float d_50_old  = abs(calc_dia - 21.0f);
                float d_50_new  = abs(calc_dia - 17.0f);

                float min_diff = min({d_100_old, d_50_old, d_50_new});
                if (min_diff == d_100_old) {
                    classified[c.index] = {100, "$100", Scalar(50, 180, 255, 255)};
                } else {
                    classified[c.index] = {50, "$50", Scalar(30, 120, 255, 255)};
                }
            }
        }

        // Dibujar resultados en pantalla y calcular totalizador
        long total = 0;
        for (int i = 0; i < (int)detectedCoins.size(); i++) {
            auto& rc = detectedCoins[i];
            auto& ci = classified[i];

            circle(rgba, Point(rc.cx, rc.cy), rc.r, ci.drawColor, 3);
            circle(rgba, Point(rc.cx, rc.cy), 4, ci.drawColor, FILLED);

            if (ci.value > 0) {
                total += ci.value;
                putText(rgba, ci.label, Point(rc.cx - rc.r/2, rc.cy + 6),
                        FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,0,255), 4);
                putText(rgba, ci.label, Point(rc.cx - rc.r/2, rc.cy + 6),
                        FONT_HERSHEY_SIMPLEX, 0.75, ci.drawColor, 2);
            }
        }

        g_totalAmount = total;

        string countStr = "Monedas: " + to_string(detectedCoins.size());
        string totalStr = "Total: $" + to_string(total);

        putText(rgba, countStr, Point(10, 42), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(0,0,0,255), 4);
        putText(rgba, countStr, Point(10, 42), FONT_HERSHEY_SIMPLEX, 1.0, Scalar(255,255,0,255), 2);
        putText(rgba, totalStr, Point(10, 82), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0,0,0,255), 5);
        putText(rgba, totalStr, Point(10, 82), FONT_HERSHEY_SIMPLEX, 1.2, Scalar(0,255,136,255), 3);
    }
}