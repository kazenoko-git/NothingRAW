package org.codeaurora.snapcam

import android.Manifest
import android.content.pm.ActivityInfo
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat

class MainActivity : ComponentActivity() {

    private var activeCameraId by mutableStateOf<String?>(null)
    private var currentZoom by mutableStateOf(1.0f)
    private var physicalLink by mutableStateOf<String?>(null)

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
                Column(modifier = Modifier.width(300.dp).background(Color.Black.copy(alpha = 0.6f)).padding(8.dp)) {
                    Text(
                        text = "NOTHING RAW PRO V5 (SPOOFED)",
                        style = MaterialTheme.typography.titleSmall,
                        color = Color.Yellow,
                        fontWeight = FontWeight.Bold
                    )
                    
                    LazyColumn(modifier = Modifier.weight(1f)) {
                        items(nativeCameras.toList()) { camera ->
                            val parts = camera.split("|")
                            val id = parts.getOrNull(0)?.trim() ?: ""
                            val isSelected = id == activeCameraId
                            
                            Column(
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .padding(vertical = 4.dp)
                                    .background(if (isSelected) Color.DarkGray else Color.Transparent)
                                    .padding(4.dp)
                            ) {
                                Text(text = "ID: $id", color = if (isSelected) Color.Cyan else Color.White, fontWeight = FontWeight.Bold, fontSize = 12.sp)
                                Text(text = camera.substringAfter("|").trim(), color = Color.LightGray, fontSize = 8.sp, lineHeight = 10.sp)
                                
                                Row(modifier = Modifier.padding(top = 4.dp)) {
                                    Button(
                                        onClick = {
                                            activeCameraId = id
                                            physicalLink = null
                                            openCamera(id)
                                        },
                                        modifier = Modifier.height(20.dp),
                                        contentPadding = PaddingValues(horizontal = 4.dp)
                                    ) {
                                        Text("OPEN", fontSize = 8.sp)
                                    }
                                    
                                    if (id == "4") {
                                        Spacer(modifier = Modifier.width(8.dp))
                                        Button(
                                            onClick = {
                                                physicalLink = "2"
                                                setPhysicalLens("2")
                                            },
                                            colors = ButtonDefaults.buttonColors(containerColor = Color.Magenta),
                                            modifier = Modifier.height(20.dp),
                                            contentPadding = PaddingValues(horizontal = 4.dp)
                                        ) {
                                            Text("FORCE ID 2", fontSize = 8.sp)
                                        }
                                    }
                                }
                            }
                            HorizontalDivider(color = Color.Gray, thickness = 0.5.dp)
                        }
                    }
                }
                
                Column(horizontalAlignment = Alignment.End, modifier = Modifier.width(200.dp)) {
                    Text(text = "FPS: 60 (FORCED)", color = Color.Green, style = MaterialTheme.typography.labelLarge)
                    Text(text = "ZOOM: ${"%.1f".format(currentZoom)}x", color = Color.White)
                    
                    Slider(
                        value = currentZoom,
                        onValueChange = { 
                            currentZoom = it
                            setZoom(it)
                        },
                        valueRange = 0.6f..10f,
                        modifier = Modifier.padding(top = 16.dp)
                    )
                    
                    if (physicalLink != null) {
                         Text(text = "LINKED TO PHYSICAL $physicalLink", color = Color.Cyan, style = MaterialTheme.typography.labelSmall)
                    }
                    Text(text = "STAB/NR BYPASSED", color = Color.Red, style = MaterialTheme.typography.labelSmall)
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
                            startPreview(holder.surface)
                        }
                        override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}
                        override fun surfaceDestroyed(holder: SurfaceHolder) {}
                    })
                }
            }
        )
    }

    external fun stringFromJNI(): String
    external fun getCameraList(): Array<String>?
    external fun openCamera(cameraId: String)
    external fun startPreview(surface: Surface)
    external fun stopCamera()
    external fun setZoom(ratio: Float)
    external fun setPhysicalLens(physicalId: String)

    companion object {
        init {
            System.loadLibrary("nothingraw")
        }
    }
}
