package com.kazenoko.nothingraw

import android.Manifest
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {

    private var activeCameraId by mutableStateOf<String?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE

        setContent {
            var hasCameraPermission by remember {
                mutableStateOf(
                    ContextCompat.checkSelfPermission(
                        this,
                        Manifest.permission.CAMERA
                    ) == PackageManager.PERMISSION_GRANTED
                )
            }

            val launcher = rememberLauncherForActivityResult(
                contract = ActivityResultContracts.RequestPermission(),
                onResult = { granted ->
                    hasCameraPermission = granted
                }
            )

            LaunchedEffect(Unit) {
                if (!hasCameraPermission) {
                    launcher.launch(Manifest.permission.CAMERA)
                }
            }

            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = Color.Black
                ) {
                    if (hasCameraPermission) {
                        val nativeCameras = getCameraList() ?: emptyArray()
                        CameraDashboard(nativeCameras)
                    } else {
                        Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                            Text("Camera permission required", color = Color.White)
                        }
                    }
                }
            }
        }
    }

    @Composable
    fun CameraDashboard(nativeCameras: Array<String>) {
        Box(modifier = Modifier.fillMaxSize()) {
            if (activeCameraId != null) {
                Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                    AspectRatioBox(ratio = 16f / 9f) {
                        CameraPreview(activeCameraId!!)
                    }
                }
            }

            Row(modifier = Modifier.fillMaxSize().padding(16.dp), horizontalArrangement = Arrangement.SpaceBetween) {
                Column {
                    Text(
                        text = "NOTHING RAW",
                        style = MaterialTheme.typography.headlineSmall,
                        color = Color.White
                    )
                    CameraSelector(nativeCameras) { id ->
                        val newId = id.split("|")[0].trim()
                        if (newId != activeCameraId) {
                            Log.i("MainActivity", "UI: Switching to camera $newId")
                            activeCameraId = newId
                            // The actual open happens in the worker thread via JNI
                            openCamera(newId)
                        }
                    }
                }
                
                Column(horizontalAlignment = Alignment.End) {
                    Text(text = "FPS: 60 (FORCED)", color = Color.Green, style = MaterialTheme.typography.labelLarge)
                    activeCameraId?.let {
                        Text(text = "SENSOR ID: $it", color = Color.White, style = MaterialTheme.typography.labelSmall)
                    }
                }
            }
        }
    }

    @Composable
    fun AspectRatioBox(ratio: Float, content: @Composable () -> Unit) {
        BoxWithConstraints(modifier = Modifier.fillMaxSize()) {
            val constraints = this.constraints
            val maxWidth = constraints.maxWidth
            val maxHeight = constraints.maxHeight
            
            val finalWidth: Int
            val finalHeight: Int
            
            if (maxWidth / maxHeight.toFloat() > ratio) {
                finalHeight = maxHeight
                finalWidth = (maxHeight * ratio).toInt()
            } else {
                finalWidth = maxWidth
                finalHeight = (maxWidth / ratio).toInt()
            }
            
            val density = LocalDensity.current
            Box(modifier = Modifier.size(
                width = with(density) { finalWidth.toDp() },
                height = with(density) { finalHeight.toDp() }
            )) {
                content()
            }
        }
    }

    @Composable
    fun CameraPreview(id: String) {
        AndroidView(
            modifier = Modifier.fillMaxSize(),
            factory = { context ->
                SurfaceView(context).apply {
                    holder.addCallback(object : SurfaceHolder.Callback {
                        override fun surfaceCreated(holder: SurfaceHolder) {
                            Log.i("MainActivity", "UI: Surface Created, starting preview")
                            startPreview(holder.surface)
                        }
                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}
                        override fun surfaceDestroyed(holder: SurfaceHolder) {
                            Log.i("MainActivity", "UI: Surface Destroyed")
                        }
                    })
                }
            },
            update = { view ->
                // This triggers when activeCameraId changes
                // If the surface already exists, we might need to re-start preview
                // but the openCamera call in the selector already triggers a Close/Open sequence in C++.
            }
        )
    }

    @Composable
    fun CameraSelector(cameras: Array<String>, onSelected: (String) -> Unit) {
        var expanded by remember { mutableStateOf(false) }
        var selectedLabel by remember { mutableStateOf("SELECT LENS") }

        Box {
            Button(onClick = { expanded = true }, colors = ButtonDefaults.buttonColors(containerColor = Color.DarkGray)) {
                Text(selectedLabel, color = Color.White)
            }
            DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                cameras.forEach { camera ->
                    DropdownMenuItem(
                        text = { Text(camera) },
                        onClick = {
                            selectedLabel = "ID " + camera.split("|")[0].trim()
                            expanded = false
                            onSelected(camera)
                        }
                    )
                }
            }
        }
    }

    external fun stringFromJNI(): String
    external fun getCameraList(): Array<String>?
    external fun openCamera(cameraId: String)
    external fun startPreview(surface: Surface)
    external fun stopCamera()

    companion object {
        init {
            System.loadLibrary("nothingraw")
        }
    }
}
