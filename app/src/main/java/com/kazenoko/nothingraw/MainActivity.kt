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
        val bruteForceCameras = mutableListOf<String>()
        
        val manager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        
        // Brute-force IDs 0-20 to find unlisted sensors
        for (i in 0..20) {
            val testId = i.toString()
            try {
                val chars = manager.getCameraCharacteristics(testId)
                val facing = chars.get(CameraCharacteristics.LENS_FACING)
                val capabilities = chars.get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)
                val isLogical = capabilities?.contains(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) ?: false
                
                var label = "ID $testId: $facing"
                if (isLogical) {
                    label += " (Logical)"
                    val physicalIds = chars.physicalCameraIds
                    label += " -> Physicals: ${physicalIds.joinToString(",")}"
                }
                bruteForceCameras.add(label)
            } catch (e: Exception) {
                // Ignore IDs that don't exist
            }
        }

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    CameraListScreen(stringFromJNI(), nativeCameras, bruteForceCameras.toTypedArray())
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
fun CameraListScreen(status: String, nativeCameras: Array<String>, bruteForceCameras: Array<String>) {
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
            text = "Native Engine (NDK + Brute):",
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
            text = "System Brute-Force (Java):",
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        LazyColumn(modifier = Modifier.weight(1f)) {
            items(bruteForceCameras.toList()) { camera ->
                CameraEntry(camera)
            }
        }
    }
}

@Composable
fun CameraEntry(camera: String) {
    val parts = camera.split(":")
    val idInfo = parts.getOrNull(0) ?: "Unknown"
    val facingCode = parts.getOrNull(1)?.trim()?.take(1)
    val facing = when (facingCode) {
        "0" -> "Front"
        "1" -> "Back"
        "2" -> "External"
        else -> "Info: $camera"
    }
    
    Column(modifier = Modifier.padding(vertical = 8.dp)) {
        Text(text = idInfo, style = MaterialTheme.typography.bodyLarge)
        if (facing != "Info: $camera") {
             Text(text = "Facing: $facing", style = MaterialTheme.typography.bodyMedium)
        }
    }
    Divider()
}
