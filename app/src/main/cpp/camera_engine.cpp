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
        commandQueue_.push({CommandType::EXIT, "", nullptr});
    }
    queueCondition_.notify_one();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void CameraEngine::OpenCamera(const std::string& id) {
    LOGI("JNI: Queueing OpenCamera %s", id.c_str());
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::OPEN, id, nullptr});
    queueCondition_.notify_one();
}

void CameraEngine::CloseCamera() {
    LOGI("JNI: Queueing CloseCamera");
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::CLOSE, "", nullptr});
    queueCondition_.notify_one();
}

void CameraEngine::StartPreview(ANativeWindow* window) {
    LOGI("JNI: Queueing StartPreview");
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::START_PREVIEW, "", window});
    queueCondition_.notify_one();
}

void CameraEngine::StopPreview() {
    LOGI("JNI: Queueing StopPreview");
    std::lock_guard<std::mutex> lock(queueMutex_);
    commandQueue_.push({CommandType::STOP_PREVIEW, "", nullptr});
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
            case CommandType::OPEN:
                LOGI("Thread: Executing OPEN %s", cmd.cameraId.c_str());
                CloseCamera_Internal();
                ACameraManager_openCamera(cameraManager_, cmd.cameraId.c_str(), &deviceCallbacks_, &cameraDevice_);
                LOGI("Thread: OPEN call finished");
                break;
            case CommandType::CLOSE:
                LOGI("Thread: Executing CLOSE");
                CloseCamera_Internal();
                LOGI("Thread: CLOSE finished");
                break;
            case CommandType::START_PREVIEW:
                LOGI("Thread: Executing START_PREVIEW");
                if (cameraDevice_ && cmd.window) {
                    window_ = cmd.window;
                    StopPreview_Internal();
                    ACaptureSessionOutputContainer_create(&outputs_);
                    ACameraOutputTarget_create(window_, &textureTarget_);
                    ACaptureSessionOutput_create(window_, &textureOutput_);
                    ACaptureSessionOutputContainer_add(outputs_, textureOutput_);

                    LOGI("Thread: Creating Session");
                    ACameraDevice_createCaptureSession(cameraDevice_, outputs_, &sessionCallbacks_, &captureSession_);

                    ACameraDevice_createCaptureRequest(cameraDevice_, TEMPLATE_PREVIEW, &previewRequest_);
                    ACaptureRequest_addTarget(previewRequest_, textureTarget_);

                    int32_t fpsRange[2] = {60, 60};
                    ACaptureRequest_setEntry_i32(previewRequest_, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, fpsRange);
                    uint8_t oisMode = ACAMERA_LENS_OPTICAL_STABILIZATION_MODE_ON;
                    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_LENS_OPTICAL_STABILIZATION_MODE, 1, &oisMode);
                    uint8_t eisMode = ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_ON;
                    ACaptureRequest_setEntry_u8(previewRequest_, ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE, 1, &eisMode);

                    LOGI("Thread: Setting Repeating Request");
                    ACameraCaptureSession_setRepeatingRequest(captureSession_, nullptr, 1, &previewRequest_, nullptr);
                } else {
                    LOGE("Thread: Cannot start preview, device: %p, window: %p", cameraDevice_, cmd.window);
                }
                LOGI("Thread: START_PREVIEW finished");
                break;
            case CommandType::STOP_PREVIEW:
                LOGI("Thread: Executing STOP_PREVIEW");
                StopPreview_Internal();
                LOGI("Thread: STOP_PREVIEW finished");
                break;
            case CommandType::EXIT:
                LOGI("Thread: Executing EXIT");
                CloseCamera_Internal();
                return;
        }
    }
}

void CameraEngine::CloseCamera_Internal() {
    StopPreview_Internal();
    if (cameraDevice_) {
        LOGI("Internal: Closing Camera Device");
        ACameraDevice_close(cameraDevice_);
        cameraDevice_ = nullptr;
    }
}

void CameraEngine::StopPreview_Internal() {
    if (captureSession_) {
        LOGI("Internal: Stopping Session");
        // Use a non-blocking stop if possible, but NDK is often synchronous here
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
    LOGI("Internal: Preview Stopped and Resources Freed");
}

// Callbacks
void CameraEngine::OnDeviceDisconnected(void* context, ACameraDevice* device) {
    LOGI("Callback: Camera Device Disconnected");
}

void CameraEngine::OnDeviceError(void* context, ACameraDevice* device, int error) {
    LOGE("Callback: Camera Device Error: %d", error);
}

void CameraEngine::OnSessionActive(void* context, ACameraCaptureSession* session) {
    LOGI("Callback: Camera Session Active");
}

void CameraEngine::OnSessionReady(void* context, ACameraCaptureSession* session) {
    LOGI("Callback: Camera Session Ready");
}

void CameraEngine::OnSessionClosed(void* context, ACameraCaptureSession* session) {
    LOGI("Callback: Camera Session Closed");
}

} // namespace nothingraw
