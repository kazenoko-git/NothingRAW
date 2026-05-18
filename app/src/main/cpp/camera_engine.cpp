#include "camera_engine.h"
#include <android/log.h>
#include <algorithm>

#define TAG "NothingRAW_CameraEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

// Forward declare to bypass header issues if any, but ensure it's linked
extern "C" void ACaptureSessionOutput_setPhysicalCameraId(ACaptureSessionOutput* output, const char* physicalId) __attribute__((weak));

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
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        isRunning_ = false;
        commandQueue_.push({CommandType::EXIT, "", nullptr, 0.0f});
    }
    queueCondition_.notify_one();
    if (workerThread_.joinable()) workerThread_.join();
}

void CameraEngine::OpenCamera(const std::string& id) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::OPEN, id, nullptr, 0.0f});
    queueCondition_.notify_one();
}

void CameraEngine::CloseCamera() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::CLOSE, "", nullptr, 0.0f});
    queueCondition_.notify_one();
}

void CameraEngine::StartPreview(ANativeWindow* window) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::START_PREVIEW, "", window, 0.0f});
    queueCondition_.notify_one();
}

void CameraEngine::StopPreview() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::STOP_PREVIEW, "", nullptr, 0.0f});
    queueCondition_.notify_one();
}

void CameraEngine::SetZoom(float ratio) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::SET_ZOOM, "", nullptr, ratio});
    queueCondition_.notify_one();
}

void CameraEngine::SetPhysicalLens(const std::string& physicalId) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::SET_PHYSICAL, physicalId, nullptr, 0.0f});
    queueCondition_.notify_one();
}

void CameraEngine::RunCommandLoop() {
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
                activeId_ = cmd.stringParam;
                CloseCamera_Internal();
                ACameraMetadata* chars = nullptr;
                ACameraManager_getCameraCharacteristics(cameraManager_, activeId_.c_str(), &chars);
                if (chars) {
                    ACameraMetadata_const_entry entry;
                    ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &entry);
                    if (entry.count == 4) {
                        activeArray_ = {entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3]};
                    }
                    ACameraMetadata_free(chars);
                }
                ACameraManager_openCamera(cameraManager_, activeId_.c_str(), &deviceCallbacks_, &cameraDevice_);
                if (window_) StartPreview_Internal();
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
                zoomRatio_ = cmd.floatParam;
                if (captureSession_ && previewRequest_) {
                    ACaptureRequest_setEntry_float(previewRequest_, ACAMERA_CONTROL_ZOOM_RATIO, 1, &zoomRatio_);
                    ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &previewRequest_, nullptr);
                }
                break;
            case CommandType::SET_PHYSICAL:
                targetPhysicalId_ = cmd.stringParam;
                if (window_ && cameraDevice_) StartPreview_Internal();
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

    // NUCLEAR OPTION: PHYSICAL ID LINKAGE (Weakly linked for safety)
    if (!targetPhysicalId_.empty() && ACaptureSessionOutput_setPhysicalCameraId != nullptr) {
        LOGI("Internal: Linking Surface to Physical ID: %s", targetPhysicalId_.c_str());
        ACaptureSessionOutput_setPhysicalCameraId(textureOutput_, targetPhysicalId_.c_str());
    }

    ACaptureSessionOutputContainer_add(outputs_, textureOutput_);

    ACameraDevice_createCaptureSession(cameraDevice_, outputs_, &sessionCallbacks_, &captureSession_);
    ACameraDevice_createCaptureRequest(cameraDevice_, TEMPLATE_PREVIEW, &previewRequest_);
    ACaptureRequest_addTarget(previewRequest_, textureTarget_);

    int32_t fpsRange[2] = {60, 60};
    ACaptureRequest_setEntry_i32(previewRequest_, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, fpsRange);

    float zoom = zoomRatio_;
    ACaptureRequest_setEntry_float(previewRequest_, ACAMERA_CONTROL_ZOOM_RATIO, 1, &zoom);

    uint8_t off = 0;
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_LENS_OPTICAL_STABILIZATION_MODE, 1, &off);
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE, 1, &off);
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_NOISE_REDUCTION_MODE, 1, &off);
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_EDGE_MODE, 1, &off);
    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_DISTORTION_CORRECTION_MODE, 1, &off);

    if (!activeArray_.empty()) {
        ACaptureRequest_setEntry_i32(previewRequest_, ACAMERA_SCALER_CROP_REGION, 4, activeArray_.data());
    }

    ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &previewRequest_, nullptr);
    LOGI("Internal: Preview started with Physical Linkage: %s", targetPhysicalId_.c_str());
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

// Callbacks
void CameraEngine::OnDeviceDisconnected(void* context, ACameraDevice* device) {}
void CameraEngine::OnDeviceError(void* context, ACameraDevice* device, int error) {}
void CameraEngine::OnSessionActive(void* context, ACameraCaptureSession* session) {}
void CameraEngine::OnSessionReady(void* context, ACameraCaptureSession* session) {}
void CameraEngine::OnSessionClosed(void* context, ACameraCaptureSession* session) {}

} // namespace nothingraw
