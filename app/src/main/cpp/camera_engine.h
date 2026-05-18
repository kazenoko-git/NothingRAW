#ifndef NOTHINGRAW_CAMERA_ENGINE_H
#define NOTHINGRAW_CAMERA_ENGINE_H

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <vector>

namespace nothingraw {

class CameraEngine {
public:
    CameraEngine(ACameraManager* manager);
    ~CameraEngine();

    void OpenCamera(const std::string& id);
    void CloseCamera();

    void StartPreview(ANativeWindow* window);
    void StopPreview();

    void SetZoom(float ratio);

private:
    void RunCommandLoop();
    void CloseCamera_Internal();
    void StartPreview_Internal();
    void StopPreview_Internal();

    ACameraManager* cameraManager_;
    ACameraDevice* cameraDevice_ = nullptr;
    ACameraCaptureSession* captureSession_ = nullptr;
    ACameraOutputTarget* textureTarget_ = nullptr;
    ACaptureSessionOutput* textureOutput_ = nullptr;
    ACaptureSessionOutputContainer* outputs_ = nullptr;
    ACaptureRequest* previewRequest_ = nullptr;
    ANativeWindow* window_ = nullptr;

    float zoomRatio_ = 1.0f;
    std::vector<int32_t> activeArray_;

    enum class CommandType { OPEN, CLOSE, START_PREVIEW, STOP_PREVIEW, SET_ZOOM, EXIT };
    struct Command {
        CommandType type;
        std::string cameraId;
        ANativeWindow* window;
        float zoomRatio;
    };
    std::queue<Command> commandQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    std::thread workerThread_;
    std::atomic<bool> isRunning_{true};

    // Callbacks
    static void OnDeviceDisconnected(void* context, ACameraDevice* device);
    static void OnDeviceError(void* context, ACameraDevice* device, int error);
    ACameraDevice_StateCallbacks deviceCallbacks_;

    static void OnSessionActive(void* context, ACameraCaptureSession* session);
    static void OnSessionReady(void* context, ACameraCaptureSession* session);
    static void OnSessionClosed(void* context, ACameraCaptureSession* session);
    ACameraCaptureSession_stateCallbacks sessionCallbacks_;
};

} // namespace nothingraw

#endif // NOTHINGRAW_CAMERA_ENGINE_H
