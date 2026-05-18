package com.kazenoko.nothingraw

import android.content.Context
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Divider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val nativeCameras = getCameraList() ?: emptyArray()
        val allCameras = mutableListOf<String>()
        
        // Use Java CameraManager to find hidden physical cameras for debugging
        val manager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        try {
            for (id in manager.cameraIdList) {
                val chars = manager.getCameraCharacteristics(id)
                val facing = chars.get(CameraCharacteristics.LENS_FACING)
                allCameras.add("Logical $id:$facing")
                
                val physicalIds = chars.physicalCameraIds
                for (pId in physicalIds) {
                    val pChars = manager.getCameraCharacteristics(pId)
                    val pFacing = pChars.get(CameraCharacteristics.LENS_FACING)
                    allCameras.add("Physical $pId:$pFacing (from $id)")
                }
            }
        } catch (e: Exception) {
            allCameras.add("Error: ${e.message}")
        }

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    CameraListScreen(stringFromJNI(), nativeCameras, allCameras.toTypedArray())
                }
            }
        }
    }

    external fun stringFromJNI(): String
    external fun getCameraList(): Array<String>?

    companion object {
        init {
            System.loadLibrary("nothingraw")
        }
    }
}

@Composable
fun CameraListScreen(status: String, nativeCameras: Array<String>, allCameras: Array<String>) {
    Column(modifier = Modifier.padding(16.dp)) {
        Text(
            text = "Nothing RAW Engine",
            style = MaterialTheme.typography.headlineMedium,
            color = MaterialTheme.colorScheme.primary
        )
        Text(
            text = status,
            style = MaterialTheme.typography.bodySmall,
            modifier = Modifier.padding(bottom = 16.dp)
        )
        
        Text(
            text = "Native Discovery (NDK):",
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        LazyColumn(modifier = Modifier.weight(1f)) {
            items(nativeCameras.toList()) { camera ->
                CameraEntry(camera)
            }
        }

        Divider(modifier = Modifier.padding(vertical = 16.dp))

        Text(
            text = "Extended Discovery (Java):",
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        LazyColumn(modifier = Modifier.weight(1f)) {
            items(allCameras.toList()) { camera ->
                CameraEntry(camera)
            }
        }
    }
}

@Composable
fun CameraEntry(camera: String) {
    val parts = camera.split(":")
    val idInfo = parts.getOrNull(0) ?: "Unknown"
    val facing = when (parts.getOrNull(1)) {
        "0" -> "Front"
        "1" -> "Back"
        "2" -> "External"
        else -> "Unknown"
    }
    
    Column(modifier = Modifier.padding(vertical = 8.dp)) {
        Text(text = "ID: $idInfo", style = MaterialTheme.typography.bodyLarge)
        Text(text = "Facing: $facing", style = MaterialTheme.typography.bodyMedium)
    }
    Divider()
}
