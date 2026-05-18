package com.kazenoko.nothingraw

import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView

class MainActivity : ComponentActivity() {

    private var activeCameraId by mutableStateOf<String?>(null)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val nativeCameras = getCameraList() ?: emptyArray()

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = Color.Black
                ) {
                    Box(modifier = Modifier.fillMaxSize()) {
                        // Camera Preview Layer
                        if (activeCameraId != null) {
                            CameraPreview(activeCameraId!!)
                        } else {
                            Box(modifier = Modifier.fillMaxSize(), contentAlignment = Alignment.Center) {
                                Text("Select a camera to start preview", color = Color.Gray)
                            }
                        }

                        // UI Overlay
                        Column(modifier = Modifier.padding(16.dp)) {
                            Text(
                                text = "Nothing RAW Engine",
                                style = MaterialTheme.typography.headlineSmall,
                                color = Color.White
                            )
                            
                            Spacer(modifier = Modifier.height(8.dp))
                            
                            CameraSelector(nativeCameras) { id ->
                                stopCamera()
                                activeCameraId = id.split("|")[0].trim()
                                openCamera(activeCameraId!!)
                            }
                        }
                        
                        // FPS Counter (Placeholder for now)
                        Text(
                            text = "Target: 60 FPS (Forced)",
                            modifier = Modifier.align(Alignment.BottomEnd).padding(16.dp),
                            color = Color.Green,
                            style = MaterialTheme.typography.labelLarge
                        )
                    }
                }
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
                            startPreview(holder.surface)
                        }
                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}
                        override fun surfaceDestroyed(holder: SurfaceHolder) {
                            stopCamera()
                        }
                    })
                }
            },
            update = { /* No-op */ }
        )
    }

    @Composable
    fun CameraSelector(cameras: Array<String>, onSelected: (String) -> Unit) {
        var expanded by remember { mutableStateOf(false) }
        var selectedLabel by remember { mutableStateOf("Select Camera") }

        Box {
            Button(onClick = { expanded = true }) {
                Text(selectedLabel)
            }
            DropdownMenu(expanded = expanded, onDismissRequest = { expanded = false }) {
                cameras.forEach { camera ->
                    DropdownMenuItem(
                        text = { Text(camera) },
                        onClick = {
                            selectedLabel = camera.split("|")[0].trim()
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
    external fun openCamera(cameraId: String): Int
    external fun startPreview(surface: Surface)
    external fun stopCamera()

    companion object {
        init {
            System.loadLibrary("nothingraw")
        }
    }
}
