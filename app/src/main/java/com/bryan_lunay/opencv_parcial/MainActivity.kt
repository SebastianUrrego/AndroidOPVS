package com.bryan_lunay.opencv_parcial

import android.Manifest
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.os.Bundle
import android.provider.MediaStore
import android.view.SurfaceView
import android.view.View
import android.view.WindowManager
import android.widget.Button
import android.widget.ImageView
import android.widget.LinearLayout
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import androidx.exifinterface.media.ExifInterface
import org.opencv.android.CameraBridgeViewBase
import org.opencv.android.JavaCamera2View
import org.opencv.android.OpenCVLoader
import org.opencv.android.Utils
import org.opencv.core.Core
import org.opencv.core.Mat
import java.io.File
import java.io.FileOutputStream

class MainActivity : AppCompatActivity(), CameraBridgeViewBase.CvCameraViewListener2 {

    private lateinit var cameraView: CameraBridgeViewBase
    private lateinit var galleryView: ImageView
    private lateinit var btnLayout: LinearLayout
    private lateinit var tvModeLabel: TextView
    private lateinit var tvTotalMoney: TextView

    private var isCameraMode = true
    // Modos:
    // 0=Normal, 1=Sketch, 2=Sepia, 3=Verde, 4=Rostros(Haar), 5=Dinero
    private var currentEffect = 0
    private var currentGalleryImage: Mat? = null
    private var cameraIndex = CameraBridgeViewBase.CAMERA_ID_BACK

    // Nombres de cada modo para mostrar en la UI
    private val modeNames = arrayOf(
        "Normal", "Sketch", "Sepia",
        "Seg. Verde", "Detec. Rostros", "Det. Monedas COP"
    )

    // ── Funciones nativas (C++) ──────────────────────────────────────
    private external fun processImageNative(matAddrRgba: Long, effectMode: Int)
    private external fun initFaceCascade(cascadePath: String)
    private external fun getDetectedAmount(): Long

    companion object {
        init {
            System.loadLibrary("opencv_parcial")
            OpenCVLoader.initDebug()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        )
        setContentView(R.layout.activity_main)

        // Inicializar vistas
        cameraView    = findViewById(R.id.camera_view)
        galleryView   = findViewById(R.id.gallery_view)
        btnLayout     = findViewById(R.id.btn_layout)
        tvModeLabel   = findViewById(R.id.tv_mode_label)
        tvTotalMoney  = findViewById(R.id.tv_total_money)

        cameraView.visibility = SurfaceView.VISIBLE
        cameraView.setCvCameraViewListener(this)
        cameraView.setCameraIndex(cameraIndex)

        // ── Copiar el cascade XML desde assets al almacenamiento interno ──
        // El archivo debe existir en app/src/main/assets/
        val cascadePath = copyAssetToInternalStorage("haarcascade_frontalface_default.xml")
        if (cascadePath != null) {
            initFaceCascade(cascadePath)
        }

        // Verificar/pedir permiso de cámara
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.CAMERA), 101)
        } else {
            cameraView.setCameraPermissionGranted()
        }

        // ── Botón CAM / GALERÍA ──────────────────────────────────────
        findViewById<Button>(R.id.btn_mode).setOnClickListener {
            isCameraMode = !isCameraMode
            if (isCameraMode) {
                galleryView.visibility = View.GONE
                cameraView.visibility = View.VISIBLE
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                    == PackageManager.PERMISSION_GRANTED) {
                    cameraView.setCameraPermissionGranted()
                    cameraView.enableView()
                }
            } else {
                cameraView.disableView()
                cameraView.visibility = View.GONE
                galleryView.visibility = View.VISIBLE
                openGallery()
            }
        }

        // ── Botón GIRAR CÁMARA ───────────────────────────────────────
        findViewById<Button>(R.id.btn_switch_cam).setOnClickListener {
            if (isCameraMode) {
                cameraView.disableView()
                cameraIndex = if (cameraIndex == CameraBridgeViewBase.CAMERA_ID_BACK)
                    CameraBridgeViewBase.CAMERA_ID_FRONT
                else
                    CameraBridgeViewBase.CAMERA_ID_BACK
                cameraView.setCameraIndex(cameraIndex)
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                    == PackageManager.PERMISSION_GRANTED) {
                    cameraView.setCameraPermissionGranted()
                    cameraView.enableView()
                }
            }
        }

        // ── Botón FILTRO / MODO ──────────────────────────────────────
        findViewById<Button>(R.id.btn_effect).setOnClickListener {
            currentEffect = (currentEffect + 1) % 6

            // Actualizar etiqueta de modo
            tvModeLabel.text = "Modo: ${modeNames[currentEffect]}"

            // Modo 4 (Rostros): forzar cámara frontal automáticamente
            if (currentEffect == 4 && isCameraMode) {
                cameraView.disableView()
                cameraIndex = CameraBridgeViewBase.CAMERA_ID_FRONT
                cameraView.setCameraIndex(cameraIndex)
                if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
                    == PackageManager.PERMISSION_GRANTED) {
                    cameraView.setCameraPermissionGranted()
                    cameraView.enableView()
                }
            }

            // Mostrar/ocultar panel de dinero
            tvTotalMoney.visibility =
                if (currentEffect == 5) View.VISIBLE else View.GONE

            // En modo galería, reaplicar efecto inmediatamente
            if (!isCameraMode && currentGalleryImage != null) {
        }
    }

    // ── Copiar un asset al almacenamiento interno ────────────────────
    // Necesario para que el código C++ pueda leer el XML del cascade
    private fun copyAssetToInternalStorage(filename: String): String? {
        return try {
            val outFile = File(filesDir, filename)
            if (!outFile.exists()) {
                assets.open(filename).use { input ->
                    FileOutputStream(outFile).use { output ->
                        input.copyTo(output)
                    }
                }
            }
            outFile.absolutePath
        } catch (e: Exception) {
            e.printStackTrace()
            null
        }
    }

    override fun onResume() {
        super.onResume()
        if (isCameraMode && ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA)
            == PackageManager.PERMISSION_GRANTED) {
            cameraView.setCameraPermissionGranted()
            cameraView.enableView()
        }
    }

    override fun onPause() {
        super.onPause()
        cameraView.disableView()
    }

    override fun onDestroy() {
        super.onDestroy()
        cameraView.disableView()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<out String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == 101 && grantResults.isNotEmpty()
            && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
            cameraView.setCameraPermissionGranted()
            if (isCameraMode) cameraView.enableView()
        }
    }

    override fun onCameraViewStarted(width: Int, height: Int) {
        (cameraView as? JavaCamera2View)?.setMaxFrameSize(1920, 1080)
    }

    override fun onCameraViewStopped() {}

    override fun onCameraFrame(inputFrame: CameraBridgeViewBase.CvCameraViewFrame): Mat {
        val rgba = inputFrame.rgba()

        // Corregir espejo en cámara frontal
        if (cameraIndex == CameraBridgeViewBase.CAMERA_ID_FRONT) {
            Core.flip(rgba, rgba, 1)
        }

        // Aplicar el efecto/modo seleccionado
        processImageNative(rgba.nativeObjAddr, currentEffect)

        // Si estamos en modo dinero, actualizar el total en el hilo UI
        if (currentEffect == 5) {
            val amount = getDetectedAmount()
            runOnUiThread {
                tvTotalMoney.text = "Total: $${ formatAmount(amount) }"
            }
        }

        return rgba
    }

    // ── Formatear monto con separadores de miles ─────────────────────
    private fun formatAmount(amount: Long): String {
        if (amount == 0L) return "0"
        val s = amount.toString()
        val result = StringBuilder()
        s.reversed().forEachIndexed { i, c ->
            if (i > 0 && i % 3 == 0) result.append('.')
            result.append(c)
        }
        return result.reverse().toString()
    }

    // ── Selector de galería ──────────────────────────────────────────
    private val galleryLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == RESULT_OK && result.data != null) {
                val imageUri = result.data!!.data!!

                val inputStream = contentResolver.openInputStream(imageUri)
                val exif = ExifInterface(inputStream!!)
                val rotation = exif.getAttributeInt(
                    ExifInterface.TAG_ORIENTATION,
                    ExifInterface.ORIENTATION_NORMAL
                )
                val degrees = when (rotation) {
                    ExifInterface.ORIENTATION_ROTATE_90  -> 90f
                    ExifInterface.ORIENTATION_ROTATE_180 -> 180f
                    ExifInterface.ORIENTATION_ROTATE_270 -> 270f
                    else -> 0f
                }
                inputStream.close()

                val inputStream2 = contentResolver.openInputStream(imageUri)
                val bitmap = BitmapFactory.decodeStream(inputStream2)
                inputStream2?.close()

                val rotatedBitmap = if (degrees != 0f) {
                    val matrix = Matrix().apply { postRotate(degrees) }
                    Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
                } else bitmap

                // Redimensionar automáticamente si la foto es muy grande (máx 1280px)
                // Esto evita que la app se cuelgue y asegura que los parámetros
                // de detección funcionen igual que con la cámara en vivo.
                val maxSize = 1280
                val finalBitmap = if (rotatedBitmap.width > maxSize || rotatedBitmap.height > maxSize) {
                    val ratio = Math.min(
                        maxSize.toFloat() / rotatedBitmap.width,
                        maxSize.toFloat() / rotatedBitmap.height
                    )
                    val width = Math.round(ratio * rotatedBitmap.width)
                    val height = Math.round(ratio * rotatedBitmap.height)
                    Bitmap.createScaledBitmap(rotatedBitmap, width, height, true)
                } else {
                    rotatedBitmap
                }

                val mat = Mat()
                val bmp32 = finalBitmap.copy(Bitmap.Config.ARGB_8888, true)
                Utils.bitmapToMat(bmp32, mat)
                currentGalleryImage = mat

                applyEffectToGalleryImage()
            }
        }

    private fun openGallery() {
        val intent = Intent(Intent.ACTION_PICK, MediaStore.Images.Media.EXTERNAL_CONTENT_URI)
        galleryLauncher.launch(intent)
    }

    private fun applyEffectToGalleryImage() {
        currentGalleryImage?.let { originalMat ->
            val matCopy = originalMat.clone()
            processImageNative(matCopy.nativeObjAddr, currentEffect)

            val resultBitmap = Bitmap.createBitmap(
                matCopy.cols(), matCopy.rows(), Bitmap.Config.ARGB_8888
            )
            Utils.matToBitmap(matCopy, resultBitmap)

            runOnUiThread {
                galleryView.scaleType = ImageView.ScaleType.FIT_CENTER
                galleryView.setImageBitmap(resultBitmap)

                // Actualizar total si estamos en modo dinero
                if (currentEffect == 5) {
                    val amount = getDetectedAmount()
                    tvTotalMoney.text = "Total: $${ formatAmount(amount) }"
                    tvTotalMoney.visibility = View.VISIBLE
                }
            }
            matCopy.release()
        }
    }
}