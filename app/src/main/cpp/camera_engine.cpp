#include "camera_engine.h"
#include <android/log.h>

#define TAG "NothingRAW_CameraEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace nothingraw {

CameraEngine::CameraEngine(ACameraManager* manager) : cameraManager_(manager) {
    deviceCallbacks_.context = this;
    deviceCallbacks_.onDisconnected = OnDeviceDisconnected;
    deviceCallbacks_.onError = OnDeviceError;

    sessionCallbacks_.context = this;
    sessionCallbacks_.onActive = OnSessionActive;
    sessionCallbacks_.onReady = OnSessionReady;
    sessionCallbacks_.onClosed = OnSessionClosed;
}

CameraEngine::~CameraEngine() {
    CloseCamera();
}

camera_status_t CameraEngine::OpenCamera(const std::string& id) {
    LOGI("Opening Camera ID: %s", id.c_str());
    return ACameraManager_openCamera(cameraManager_, id.c_str(), &deviceCallbacks_, &cameraDevice_);
}

void CameraEngine::CloseCamera() {
    StopPreview();
    if (cameraDevice_) {
        ACameraDevice_close(cameraDevice_);
        cameraDevice_ = nullptr;
    }
}

camera_status_t CameraEngine::StartPreview(ANativeWindow* window) {
    if (!cameraDevice_ || !window) return ACAMERA_ERROR_INVALID_PARAMETER;
    window_ = window;

    // 1. Create Output Container
    ACaptureSessionOutputContainer_create(&outputs_);

    // 2. Prepare Output for Surface
    ACameraOutputTarget_create(window, &textureTarget_);
    ACaptureSessionOutput_create(window, &textureOutput_);
    ACaptureSessionOutputContainer_add(outputs_, textureOutput_);

    // 3. Create Capture Session
    ACameraDevice_createCaptureSession(cameraDevice_, outputs_, &sessionCallbacks_, &captureSession_);

    // 4. Create Preview Request
    ACameraDevice_createCaptureRequest(cameraDevice_, TEMPLATE_PREVIEW, &previewRequest_);
    ACaptureRequest_addTarget(previewRequest_, textureTarget_);

    // CRITICAL: FORCE UNLOCK 60 FPS
    // Even if not in available ranges, we inject it here.
    int32_t fpsRange[2] = {60, 60};
    ACaptureRequest_setEntry_i32(previewRequest_, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, fpsRange);

    // 5. Start Repeating Request
    return ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &previewRequest_, nullptr);
}

void CameraEngine::StopPreview() {
    if (captureSession_) {
        ACameraCaptureSession_stopRepeating(captureSession_);
        ACameraCaptureSession_close(captureSession_);
        captureSession_ = nullptr;
    }
    if (previewRequest_) {
        ACaptureRequest_free(previewRequest_);
        previewRequest_ = nullptr;
    }
    if (outputs_) {
        ACaptureSessionOutputContainer_free(outputs_);
        outputs_ = nullptr;
    }
    if (textureOutput_) {
        ACaptureSessionOutput_free(textureOutput_);
        textureOutput_ = nullptr;
    }
    if (textureTarget_) {
        ACameraOutputTarget_free(textureTarget_);
        textureTarget_ = nullptr;
    }
}

// Callbacks implementation
void CameraEngine::OnDeviceDisconnected(void* context, ACameraDevice* device) {
    LOGI("Camera Device Disconnected");
}

void CameraEngine::OnDeviceError(void* context, ACameraDevice* device, int error) {
    LOGE("Camera Device Error: %d", error);
}

void CameraEngine::OnSessionActive(void* context, ACameraCaptureSession* session) {
    LOGI("Camera Session Active");
}

void CameraEngine::OnSessionReady(void* context, ACameraCaptureSession* session) {
    LOGI("Camera Session Ready");
}

void CameraEngine::OnSessionClosed(void* context, ACameraCaptureSession* session) {
    LOGI("Camera Session Closed");
}

} // namespace nothingraw
