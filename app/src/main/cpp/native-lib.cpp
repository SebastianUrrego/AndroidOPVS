#include <algorithm>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <cmath>
#include <jni.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <string>
#include <vector>

#define LOG_TAG "NativeLib"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

using namespace cv;
using namespace std;

static CascadeClassifier g_faceCascade;
static long g_totalAmount = 0;

struct RefCoin {
  long value;
  int minRadius;
  int maxRadius;
};

static vector<RefCoin> g_calibratedCoins;

extern "C" JNIEXPORT void JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_calibrateCoins(
    JNIEnv *env, jobject, jobject assetManager) {
  AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
  if (!mgr)
    return;

  struct AssetDef {
    long val;
    string file;
  };
  // Radios medidos experimentalmente en imágenes 1200×1600 px (escala ~7.1 px/mm):
  //   $50 nueva: 61px | $100 nueva: 71px | $100 vieja: 84px
  //   $200 nueva: 82px | $200 vieja: 89px | $500 nueva: 83px
  //   $500 vieja: 86px | $1000: 95px  → rango: 61–95 px (5.1%–7.9% del ancho)
  // NOTA: 50_vieja_frente/trasera.jpeg no están disponibles en assets/
  vector<AssetDef> assets = {
      {1000, "1000_frente.jpeg"},       {1000, "1000_trasera.jpeg"},
      {500,  "500_nueva_frente.jpeg"},  {500,  "500_nueva_trasera.jpeg"},
      {500,  "500_vieja_frente.jpeg"},  {500,  "500_vieja_trasera.jpeg"},
      {200,  "200_nueva_frente.jpeg"},  {200,  "200_nueva_trasera.jpeg"},
      {200,  "200_vieja_frente.jpeg"},  {200,  "200_vieja_trasera.jpeg"},
      {100,  "100_nueva_frente.jpeg"},  {100,  "100_nueva_trasera.jpeg"},
      {100,  "100_vieja_frente.jpeg"},  {100,  "100_vieja_trasera.jpeg"},
      {50,   "50_nueva_frente.jpeg"},   {50,   "50_nueva_trasera.jpeg"}};

  g_calibratedCoins.clear();

  for (const auto &ad : assets) {
    AAsset *asset =
        AAssetManager_open(mgr, ad.file.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
      LOGD("No se pudo abrir asset: %s", ad.file.c_str());
      continue;
    }

    off_t length = AAsset_getLength(asset);
    const void *data = AAsset_getBuffer(asset);
    if (!data) {
      AAsset_close(asset);
      continue;
    }

    vector<char> buffer((const char *)data, (const char *)data + length);
    AAsset_close(asset);

    Mat img = imdecode(buffer, IMREAD_COLOR);
    if (img.empty())
      continue;

    Mat gray, blurred;
    cvtColor(img, gray, COLOR_BGR2GRAY);
    GaussianBlur(gray, blurred, Size(9, 9), 2.0);
    medianBlur(blurred, blurred, 5);

    // Rango ajustado a los radios reales medidos en imágenes de referencia 1200px:
    //   mínimo: 61px ($50 nueva, 17mm) = 5.1% → se usa 5% con margen 2px
    //   máximo: 95px ($1000, 26.7mm)   = 7.9% → se usa 9% para incluir variaciones
    int minR = (int)(img.cols * 0.05f);
    int maxR = (int)(img.cols * 0.09f);

    vector<Vec3f> circles;
    HoughCircles(blurred, circles, HOUGH_GRADIENT, 1.5, img.cols * 0.06f, 100,
                 60, minR, maxR);

    if (!circles.empty()) {
      int r = cvRound(circles[0][2]);
      RefCoin rc;
      rc.value = ad.val;
      rc.minRadius =
          r - 15; // Margen de error tolerado en px (debido a distancia/enfoque)
      rc.maxRadius = r + 15;
      g_calibratedCoins.push_back(rc);
      LOGD("Calibrated %s: radius %d (min: %d, max: %d)", ad.file.c_str(), r,
           rc.minRadius, rc.maxRadius);
    }
  }
}

// ══════════════════════════════════════════════════════════════
// INICIALIZAR HAAR CASCADE
// ══════════════════════════════════════════════════════════════
extern "C" JNIEXPORT void JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_initFaceCascade(
    JNIEnv *env, jobject, jstring cascade_path) {
  const char *path = env->GetStringUTFChars(cascade_path, nullptr);
  g_faceCascade.load(path);
  env->ReleaseStringUTFChars(cascade_path, path);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_getDetectedAmount(JNIEnv *,
                                                                     jobject) {
  return (jlong)g_totalAmount;
}

// ══════════════════════════════════════════════════════════════
// AUXILIAR — Muestrea el color HSV mediano en una corona circular
// ══════════════════════════════════════════════════════════════
struct HSVStats {
  double H, S, V;
};

static HSVStats getRingHSV(const Mat &rgba, int cx, int cy, int r, float minPct,
                           float maxPct) {
  Mat bgr, hsv;
  cvtColor(rgba, bgr, COLOR_RGBA2BGR);
  cvtColor(bgr, hsv, COLOR_BGR2HSV);

  int rMin = (int)(r * minPct);
  int rMax = (int)(r * maxPct);

  vector<double> vH, vS, vV;
  vH.reserve(400);
  vS.reserve(400);
  vV.reserve(400);

  for (int dy = -rMax; dy <= rMax; dy += 2) {
    for (int dx = -rMax; dx <= rMax; dx += 2) {
      int distSq = dx * dx + dy * dy;
      if (distSq < rMin * rMin || distSq > rMax * rMax)
        continue;
      int px = cx + dx, py = cy + dy;
      if (px < 0 || py < 0 || px >= rgba.cols || py >= rgba.rows)
        continue;
      Vec3b p = hsv.at<Vec3b>(py, px);
      vH.push_back(p[0]);
      vS.push_back(p[1]);
      vV.push_back(p[2]);
    }
  }
  if (vH.empty())
    return {0, 0, 0};

  size_t mid = vH.size() / 2;
  nth_element(vH.begin(), vH.begin() + mid, vH.end());
  nth_element(vS.begin(), vS.begin() + mid, vS.end());
  nth_element(vV.begin(), vV.begin() + mid, vV.end());
  return {vH[mid], vS[mid], vV[mid]};
}

// ══════════════════════════════════════════════════════════════
// FUNCIÓN PRINCIPAL
// ══════════════════════════════════════════════════════════════
extern "C" JNIEXPORT void JNICALL
Java_com_bryan_1lunay_opencv_1parcial_MainActivity_processImageNative(
    JNIEnv *, jobject, jlong mat_addr_rgba, jint effect_mode) {

  Mat &rgba = *(Mat *)mat_addr_rgba;

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
    Mat kernel = (Mat_<float>(4, 4) << 0.272, 0.534, 0.131, 0, 0.349, 0.686,
                  0.168, 0, 0.393, 0.769, 0.189, 0, 0, 0, 0, 1);
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
      g_faceCascade.detectMultiScale(gray, faces, 1.05, 4, CASCADE_SCALE_IMAGE,
                                     Size(60, 60),
                                     Size(rgba.cols / 2, rgba.rows / 2));

      for (const Rect &face : faces) {
        rectangle(rgba, face, Scalar(0, 255, 0, 255), 2);
        int len = face.width / 5;
        line(rgba, face.tl(), {face.x + len, face.y}, Scalar(0, 255, 0, 255),
             4);
        line(rgba, face.tl(), {face.x, face.y + len}, Scalar(0, 255, 0, 255),
             4);
        line(rgba, {face.x + face.width, face.y},
             {face.x + face.width - len, face.y}, Scalar(0, 255, 0, 255), 4);
        line(rgba, {face.x + face.width, face.y},
             {face.x + face.width, face.y + len}, Scalar(0, 255, 0, 255), 4);
        line(rgba, {face.x, face.y + face.height},
             {face.x + len, face.y + face.height}, Scalar(0, 255, 0, 255), 4);
        line(rgba, {face.x, face.y + face.height},
             {face.x, face.y + face.height - len}, Scalar(0, 255, 0, 255), 4);
        line(rgba, face.br(), {face.x + face.width - len, face.y + face.height},
             Scalar(0, 255, 0, 255), 4);
        line(rgba, face.br(), {face.x + face.width, face.y + face.height - len},
             Scalar(0, 255, 0, 255), 4);

        string label = "Rostro";
        int baseline = 0;
        Size ts = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);
        Rect bgRect(face.x, face.y - ts.height - 8, ts.width + 8,
                    ts.height + 8);
        if (bgRect.y >= 0)
          rectangle(rgba, bgRect, Scalar(0, 0, 0, 180), FILLED);
        putText(rgba, label, Point(face.x + 4, face.y - 4),
                FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0, 255), 2);
      }

      string txt = "Rostros: " + to_string(faces.size());
      putText(rgba, txt, Point(10, 38), FONT_HERSHEY_SIMPLEX, 1.0,
              Scalar(0, 0, 0, 255), 4);
      putText(rgba, txt, Point(10, 38), FONT_HERSHEY_SIMPLEX, 1.0,
              Scalar(0, 255, 0, 255), 2);
    } else {
      putText(rgba, "Cascade no disponible", Point(10, 40),
              FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255, 255), 2);
    }

    // ══════════════════════════════════════════════════════════════
    // MODO 5: DETECCIÓN DE MONEDAS COLOMBIANAS
    //
    // Diámetros físicos reales (Banco de la República de Colombia):
    //   $50  nueva: 17.0 mm   vieja: 21.0 mm
    //   $100 nueva: 20.3 mm   vieja: 23.0 mm
    //   $200 nueva: 22.4 mm   vieja: 24.4 mm
    //   $500      : 23.7 mm   (bimetálica: centro dorado, borde plateado)
    //   $1000     : 26.7 mm   (bimetálica: borde dorado, centro plateado)
    //
    // Estrategia:
    //   1. Detectar bimetálicas por color HSV centro vs borde.
    //   2. Priorizar $500 como ancla (mismo tamaño en ambas familias) → px_to_mm.
    //   3. Clasificar todas las monedas por color (Hue+Sat) + diámetro en mm.
    //   4. Sin bimetálicas: escala estimada por la mediana de los círculos.
    // ══════════════════════════════════════════════════════════════
  } else if (effect_mode == 5) {

    // PASO 1: Preprocesar
    Mat gray, blurred;
    cvtColor(rgba, gray, COLOR_RGBA2GRAY);
    GaussianBlur(gray, blurred, Size(9, 9), 2.0);
    medianBlur(blurred, blurred, 5);

    // PASO 2: HoughCircles — rango calculado a partir de radios medidos experimentalmente
    // Referencia (imgs 1200px, ~7.1 px/mm): $50 nueva=61px…$1000=95px (5.1%–7.9%)
    // minR=4% cubre monedas a ~25% más lejos que la distancia de referencia (~40 cm).
    // maxR=30% cubre monedas a ~4x más cerca que la referencia (~8 cm, máximo práctico).
    // param2=115 estricto: reduce falsos positivos sobre superficies texturizadas.
    int minR = (int)(rgba.cols * 0.04f);
    int maxR = (int)(rgba.cols * 0.30f);

    vector<Vec3f> rawCircles;
    HoughCircles(
        blurred, rawCircles, HOUGH_GRADIENT, 1.5,
        rgba.cols * 0.10f,
        100,
        115,
        minR, maxR);

    // PASO 3: NMS
    vector<Vec3f> coins;
    for (const auto &c : rawCircles) {
      bool keep = true;
      for (const auto &a : coins) {
        float d = hypot(c[0] - a[0], c[1] - a[1]);
        if (d < (c[2] + a[2]) * 0.40f) {
          keep = false;
          break;
        }
      }
      if (keep)
        coins.push_back(c);
    }

    if (coins.empty()) {
      g_totalAmount = 0;
      return;
    }

    // PASO 4: Analizar color HSV de cada círculo
    struct Coin {
      int cx, cy, r;
      double cS, eS, cH, eH;
      bool is500, is1000, isGold;
    };
    vector<Coin> detected;

    for (const auto &c : coins) {
      int cx = cvRound(c[0]), cy = cvRound(c[1]), r = cvRound(c[2]);
      if (cx - r < 0 || cy - r < 0 || cx + r >= rgba.cols ||
          cy + r >= rgba.rows)
        continue;

      // Restaurado a 0.40 y 0.90 para que las bimetálicas ($500 y $1000) detecten su anillo.
      HSVStats cen = getRingHSV(rgba, cx, cy, r, 0.00f, 0.40f);
      HSVStats edg = getRingHSV(rgba, cx, cy, r, 0.68f, 0.90f);
      double cS = cen.S, eS = edg.S;
      double cH = cen.H, eH = edg.H;

      // Bimetálicas: contraste de saturación fuerte entre centro y borde
      // $1000: borde dorado (alta S), centro plateado (baja S)
      bool is1000 = (eS > cS + 10.0 && eS > 20.0);
      // $500:  centro dorado (alta S), borde plateado (baja S)
      bool is500 = (cS > eS + 10.0 && cS > 20.0);

      // Monedas doradas (Latón/Bronce) vs Plateadas (Alpaca/Acero)
      double avgS = (cS + eS) / 2.0;
      double avgH = (cH + eH) / 2.0;

      // isGold: solo monedas de latón/bronce REAL (familia vieja: $50 y $100 viejas).
      // isGold: monedas doradas (bronce/latón).
      // $100 (vieja y nueva) y $50 (vieja) son doradas.
      // $200 y $50 nueva son plateadas (alpaca/acero).
      // AUMENTAMOS la saturación a 50.0: bajo luz amarilla, una moneda plateada
      // puede reflejar luz amarilla, pero su saturación será baja (20-40).
      // Solo las monedas realmente doradas tendrán saturación > 50.
      bool isGold =
          (!is500 && !is1000 && avgS > 50.0 && avgH > 10.0 && avgH < 45.0);

      detected.push_back({cx, cy, r, cS, eS, cH, eH, is500, is1000, isGold});
    }

    if (detected.empty()) {
      g_totalAmount = 0;
      return;
    }

    // PASO 5: Estimación de escala relativa (px_to_mm) en vivo
    // Priorizar $500 (23.7 mm) — tamaño idéntico en AMBAS familias (1994 y 2012)
    float px_to_mm = -1.0f;
    {
      float sum = 0.0f;
      int cnt = 0;
      for (const auto &coin : detected)
        if (coin.is500) {
          sum += (float)coin.r / 11.85f; // radio = diámetro/2 = 23.7/2
          cnt++;
        }
      if (cnt > 0)
        px_to_mm = sum / cnt;
    }
    if (px_to_mm < 0) {
      // Segunda opción: $1000 (26.7 mm)
      float sum = 0.0f;
      int cnt = 0;
      for (const auto &coin : detected)
        if (coin.is1000) {
          sum += (float)coin.r / 13.35f;
          cnt++;
        }
      if (cnt > 0)
        px_to_mm = sum / cnt;
    }
    if (px_to_mm < 0) {
      // Fallback: Si no hay monedas bimetálicas ($500 o $1000) para anclar la escala,
      // es imposible saber físicamente a qué distancia está la cámara.
      // En lugar de "adivinar" forzando el tamaño en escalones (lo que causa que una
      // $200 salte a $100 al mover un poco la cámara), asumimos una distancia nominal
      // (ej. un campo visual de 140 mm de ancho).
      // Al hacer esto, la escala es constante. El usuario verá en pantalla cómo el diámetro 
      // calculado cambia suavemente al acercar o alejar el celular, permitiéndole ajustar 
      // la distancia de forma natural hasta que la moneda marque su tamaño real.
      px_to_mm = rgba.cols / 140.0f;
    }

    // PASO 6: Clasificar cada moneda por color y diámetro físico calculado
    auto classify = [&](int r, bool is1000_, bool is500_,
                        bool isGold_) -> pair<long, string> {
      if (is1000_)
        return make_pair(1000L, string("$1.000"));
      if (is500_)
        return make_pair(500L, string("$500"));

      long bestVal = 0;
      // Diámetro calculado en mm reales (escala relativa independiente de distancia)
      float dia = (r * 2.0f) / px_to_mm;

      if (isGold_) {
        // Monedas DORADAS:
        // - $100 vieja: 23.0 mm
        // - $100 nueva: 20.3 mm
        // - $50  vieja: 20.0 mm
        // Usamos ramas por color porque dan una tolerancia gigante a la distancia.
        // Todo lo que sea > 21.65 será la 100 vieja, sin importar si mide 22 o 25.
        if (dia > 21.65f)
          bestVal = 100L; // 23.0 mm ($100 vieja)
        else if (dia > 20.15f)
          bestVal = 100L; // 20.3 mm ($100 nueva)
        else if (dia > 18.5f)
          bestVal = 50L;  // 20.0 mm ($50 vieja)
        else
          bestVal = 50L;  // < 18.5 mm ($50 nueva que entró por error de color)
      } else {
        // Monedas PLATEADAS:
        // - $200 vieja: 24.4 mm
        // - $200 nueva: 22.4 mm
        // - $50  nueva: 17.0 mm
        if (dia > 23.4f)
          bestVal = 200L; // 24.4 mm ($200 vieja)
        else if (dia > 19.7f)
          bestVal = 200L; // 22.4 mm ($200 nueva)
        else
          bestVal = 50L;  // 17.0 mm ($50 nueva)
      }

      string lbl = "$" + to_string(bestVal);
      if (bestVal == 1000)
        lbl = "$1.000";
      return make_pair(bestVal, lbl);
    };

    // PASO 7: Dibujar resultados
    auto getColor = [](long val) -> Scalar {
      switch (val) {
      case 1000:
        return Scalar(255, 180, 50, 255);
      case 500:
        return Scalar(0, 200, 255, 255);
      case 200:
        return Scalar(220, 220, 220, 255);
      case 100:
        return Scalar(50, 160, 255, 255);
      default:
        return Scalar(30, 100, 200, 255);
      }
    };

    long total = 0;
    int coinCount = 0;

    for (const auto &coin : detected) {
      pair<long, string> res =
          classify(coin.r, coin.is1000, coin.is500, coin.isGold);
      long val = res.first;
      string lbl = res.second;
      Scalar col = getColor(val);

      // ── Diámetro calculado en mm (para debug/calibración) ──────────
      float dia_mm = (coin.r * 2.0f) / px_to_mm;
      // Formateamos a 1 decimal: "22.4mm"
      char diaBuf[16];
      snprintf(diaBuf, sizeof(diaBuf), "%.1fmm", dia_mm);

      // Color de borde: amarillo si isGold, blanco si plateada
      Scalar borderCol = coin.isGold ? Scalar(0, 230, 255, 255)   // amarillo → dorada
                                     : Scalar(220, 220, 220, 255); // blanco  → plateada

      circle(rgba, Point(coin.cx, coin.cy), coin.r, borderCol, 3);
      circle(rgba, Point(coin.cx, coin.cy), 5, col, FILLED);

      // Línea 1: valor clasificado  (ej: "$200")
      int txtX = coin.cx - coin.r / 2;
      putText(rgba, lbl, Point(txtX, coin.cy - 4),
              FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 0, 255), 4);
      putText(rgba, lbl, Point(txtX, coin.cy - 4),
              FONT_HERSHEY_SIMPLEX, 0.75, col, 2);

      // Línea 2: diámetro calculado (ej: "22.4mm") — para calibración
      putText(rgba, diaBuf, Point(txtX, coin.cy + 22),
              FONT_HERSHEY_SIMPLEX, 0.60, Scalar(0, 0, 0, 255), 3);
      putText(rgba, diaBuf, Point(txtX, coin.cy + 22),
              FONT_HERSHEY_SIMPLEX, 0.60, Scalar(0, 255, 200, 255), 1);

      // Log para Logcat: valor | dia_mm | S_centro | S_borde | isGold
      LOGD("COIN: %s | dia=%.1fmm | cS=%.1f eS=%.1f | isGold=%d is500=%d is1000=%d",
           lbl.c_str(), dia_mm,
           (float)coin.cS, (float)coin.eS,
           (int)coin.isGold, (int)coin.is500, (int)coin.is1000);

      if (val > 0) {
        total += val;
        coinCount++;
      }
    }

    // PASO 8: HUD
    g_totalAmount = total;
    string countStr = "Monedas: " + to_string(coinCount);
    string totalStr = "Total: $" + to_string(total);

    putText(rgba, countStr, Point(10, 42), FONT_HERSHEY_SIMPLEX, 1.0,
            Scalar(0, 0, 0, 255), 4);
    putText(rgba, countStr, Point(10, 42), FONT_HERSHEY_SIMPLEX, 1.0,
            Scalar(255, 255, 0, 255), 2);
    putText(rgba, totalStr, Point(10, 82), FONT_HERSHEY_SIMPLEX, 1.2,
            Scalar(0, 0, 0, 255), 5);
    putText(rgba, totalStr, Point(10, 82), FONT_HERSHEY_SIMPLEX, 1.2,
            Scalar(0, 255, 136, 255), 3);
  }
}
