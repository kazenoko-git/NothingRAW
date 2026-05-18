#ifndef NOTHINGRAW_CAMERA_ENGINE_H
#define NOTHINGRAW_CAMERA_ENGINE_H

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <string>

namespace nothingraw {

class CameraEngine {
public:
    CameraEngine(ACameraManager* manager);
    ~CameraEngine();

    camera_status_t OpenCamera(const std::string& id);
    void CloseCamera();

    camera_status_t StartPreview(ANativeWindow* window);
    void StopPreview();

private:
    ACameraManager* cameraManager_;
    ACameraDevice* cameraDevice_ = nullptr;
    ACameraCaptureSession* captureSession_ = nullptr;
    ACameraOutputTarget* textureTarget_ = nullptr;
    ACaptureSessionOutput* textureOutput_ = nullptr;
    ACaptureSessionOutputContainer* outputs_ = nullptr;
    ACaptureRequest* previewRequest_ = nullptr;
    ANativeWindow* window_ = nullptr;

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
