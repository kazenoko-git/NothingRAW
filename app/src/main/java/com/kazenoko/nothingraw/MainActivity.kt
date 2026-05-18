package com.kazenoko.nothingraw

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

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    CameraListScreen(stringFromJNI(), nativeCameras)
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
fun CameraListScreen(status: String, nativeCameras: Array<String>) {
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
            text = "Sensor Forensics (ID | Facing | Focal | Sensor Size | FPS):",
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        LazyColumn(modifier = Modifier.fillMaxSize()) {
            items(nativeCameras.toList()) { camera ->
                val parts = camera.split("|")
                val id = parts.getOrNull(0)?.trim() ?: ""
                val facing = when (parts.getOrNull(1)?.trim()) {
                    "0" -> "Front"
                    "1" -> "Back"
                    "2" -> "External"
                    else -> "Unknown"
                }
                val optics = parts.getOrNull(2)?.trim() ?: ""
                val sensor = parts.getOrNull(3)?.trim() ?: ""
                val fps = parts.getOrNull(4)?.trim() ?: ""
                
                Column(modifier = Modifier.padding(vertical = 8.dp)) {
                    Text(text = "ID: $id ($facing)", style = MaterialTheme.typography.bodyLarge, color = MaterialTheme.colorScheme.secondary)
                    Text(text = optics, style = MaterialTheme.typography.bodyMedium)
                    Text(text = sensor, style = MaterialTheme.typography.bodyMedium)
                    Text(text = fps, style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.tertiary)
                }
                Divider()
            }
        }
    }
}
