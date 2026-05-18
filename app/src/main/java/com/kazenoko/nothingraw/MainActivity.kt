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

        val cameraList = getCameraList() ?: emptyArray()

        setContent {
            MaterialTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    CameraListScreen(stringFromJNI(), cameraList)
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
fun CameraListScreen(status: String, cameras: Array<String>) {
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
            text = "Discovered Cameras:",
            style = MaterialTheme.typography.titleLarge,
            modifier = Modifier.padding(bottom = 8.dp)
        )

        LazyColumn {
            items(cameras.toList()) { camera ->
                val parts = camera.split(":")
                val id = parts.getOrNull(0) ?: "Unknown"
                val facing = when (parts.getOrNull(1)) {
                    "0" -> "Front"
                    "1" -> "Back"
                    "2" -> "External"
                    else -> "Unknown"
                }
                
                Column(modifier = Modifier.padding(vertical = 8.dp)) {
                    Text(text = "Camera ID: $id", style = MaterialTheme.typography.bodyLarge)
                    Text(text = "Facing: $facing", style = MaterialTheme.typography.bodyMedium)
                }
                Divider()
            }
        }
    }
}
