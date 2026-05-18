#include "camera_engine.h"
#include <android/log.h>

#define TAG "NothingRAW_CameraEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace nothingraw {

CameraEngine::CameraEngine(ACameraManager* manager) : cameraManager_(manager) {
    LOGI("Engine: Initializing");
    deviceCallbacks_.context = this;
    deviceCallbacks_.onDisconnected = OnDeviceDisconnected;
    deviceCallbacks_.onError = OnDeviceError;

    sessionCallbacks_.context = this;
    sessionCallbacks_.onActive = OnSessionActive;
    sessionCallbacks_.onReady = OnSessionReady;
    sessionCallbacks_.onClosed = OnSessionClosed;

    workerThread_ = std::thread(&CameraEngine::RunCommandLoop, this);
}

CameraEngine::~CameraEngine() {
    LOGI("Engine: Destroying");
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        isRunning_ = false;
        commandQueue_.push({CommandType::EXIT, "", nullptr, 1.0f});
    }
    queueCondition_.notify_one();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void CameraEngine::OpenCamera(const std::string& id) {
    LOGI("JNI: Queueing OpenCamera %s", id.c_str());
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::OPEN, id, nullptr, 1.0f});
    queueCondition_.notify_one();
}

void CameraEngine::CloseCamera() {
    LOGI("JNI: Queueing CloseCamera");
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::CLOSE, "", nullptr, 1.0f});
    queueCondition_.notify_one();
}

void CameraEngine::StartPreview(ANativeWindow* window) {
    LOGI("JNI: Queueing StartPreview");
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::START_PREVIEW, "", window, 1.0f});
    queueCondition_.notify_one();
}

void CameraEngine::StopPreview() {
    LOGI("JNI: Queueing StopPreview");
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::STOP_PREVIEW, "", nullptr, 1.0f});
    queueCondition_.notify_one();
}

void CameraEngine::SetZoom(float ratio) {
    LOGI("JNI: Queueing SetZoom %f", ratio);
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::SET_ZOOM, "", nullptr, ratio});
    queueCondition_.notify_one();
}

void CameraEngine::RunCommandLoop() {
    LOGI("Thread: Worker started");
    while (isRunning_) {
        Command cmd;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCondition_.wait(lock, [this] { return !commandQueue_.empty(); });
            cmd = std::move(commandQueue_.front());
            commandQueue_.pop();
        }

        switch (cmd.type) {
            case CommandType::OPEN: {
                LOGI("Thread: Executing OPEN %s", cmd.cameraId.c_str());
                CloseCamera_Internal();

                ACameraMetadata* chars = nullptr;
                ACameraManager_getCameraCharacteristics(cameraManager_, cmd.cameraId.c_str(), &chars);
                if (chars) {
                    ACameraMetadata_const_entry entry;
                    ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &entry);
                    if (entry.count == 4) {
                        activeArray_ = {entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3]};
                    }
                    ACameraMetadata_free(chars);
                }

                camera_status_t status = ACameraManager_openCamera(cameraManager_, cmd.cameraId.c_str(), &deviceCallbacks_, &cameraDevice_);
                if (status == ACAMERA_OK && window_) {
                    StartPreview_Internal();
                }
                break;
            }
            case CommandType::CLOSE:
                CloseCamera_Internal();
                break;
            case CommandType::START_PREVIEW:
                if (cmd.window) {
                    window_ = cmd.window;
                    if (cameraDevice_) StartPreview_Internal();
                }
                break;
            case CommandType::STOP_PREVIEW:
                StopPreview_Internal();
                break;
            case CommandType::SET_ZOOM:
                zoomRatio_ = cmd.zoomRatio;
                if (captureSession_ && previewRequest_) {
                    ACaptureRequest_setEntry_float(previewRequest_, ACAMERA_CONTROL_ZOOM_RATIO, 1, &zoomRatio_);
                    ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &previewRequest_, nullptr);
                }
                break;
            case CommandType::EXIT:
                CloseCamera_Internal();
                return;
        }
    }
}

void CameraEngine::CloseCamera_Internal() {
    StopPreview_Internal();
    if (cameraDevice_) {
        ACameraDevice_close(cameraDevice_);
        cameraDevice_ = nullptr;
    }
}

void CameraEngine::StartPreview_Internal() {
    if (!cameraDevice_ || !window_) return;

    StopPreview_Internal();

    ACaptureSessionOutputContainer_create(&outputs_);
    ACameraOutputTarget_create(window_, &textureTarget_);
    ACaptureSessionOutput_create(window_, &textureOutput_);
    ACaptureSessionOutputContainer_add(outputs_, textureOutput_);

    ACameraDevice_createCaptureSession(cameraDevice_, outputs_, &sessionCallbacks_, &captureSession_);
    ACameraDevice_createCaptureRequest(cameraDevice_, TEMPLATE_PREVIEW, &previewRequest_);
    ACaptureRequest_addTarget(previewRequest_, textureTarget_);

    // FORCED UNLOCKS
    int32_t fpsRange[2] = {60, 60};
    ACaptureRequest_setEntry_i32(previewRequest_, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, fpsRange);

    float zoom = zoomRatio_;
    ACaptureRequest_setEntry_float(previewRequest_, ACAMERA_CONTROL_ZOOM_RATIO, 1, &zoom);

    uint8_t oisMode = ACAMERA_LENS_OPTICAL_STABILIZATION_MODE_ON;
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_LENS_OPTICAL_STABILIZATION_MODE, 1, &oisMode);

    uint8_t noiseMode = ACAMERA_NOISE_REDUCTION_MODE_OFF;
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_NOISE_REDUCTION_MODE, 1, &noiseMode);

    ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &previewRequest_, nullptr);
}

void CameraEngine::StopPreview_Internal() {
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

// Callbacks (Simplified for brevity)
void CameraEngine::OnDeviceDisconnected(void* context, ACameraDevice* device) {}
void CameraEngine::OnDeviceError(void* context, ACameraDevice* device, int error) {}
void CameraEngine::OnSessionActive(void* context, ACameraCaptureSession* session) {}
void CameraEngine::OnSessionReady(void* context, ACameraCaptureSession* session) {}
void CameraEngine::OnSessionClosed(void* context, ACameraCaptureSession* session) {}

} // namespace nothingraw
