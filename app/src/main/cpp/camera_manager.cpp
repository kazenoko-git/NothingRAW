#include "camera_manager.h"
#include <android/log.h>

#define TAG "NothingRAW_CameraManager"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace nothingraw {

CameraManager::CameraManager() {
    cameraManager_ = ACameraManager_create();
}

CameraManager::~CameraManager() {
    if (cameraManager_) {
        ACameraManager_delete(cameraManager_);
        cameraManager_ = nullptr;
    }
}

std::vector<CameraInfo> CameraManager::GetCameraList() {
    std::vector<CameraInfo> cameras;
    ACameraIdList* cameraIdList = nullptr;
    camera_status_t status = ACameraManager_getCameraIdList(cameraManager_, &cameraIdList);

    if (status != ACAMERA_OK || cameraIdList == nullptr) {
        LOGE("Failed to get camera ID list, status: %d", status);
        return cameras;
    }

    for (int i = 0; i < cameraIdList->numCameras; ++i) {
        const char* id = cameraIdList->cameraIds[i];

        ACameraMetadata* chars = nullptr;
        status = ACameraManager_getCameraCharacteristics(cameraManager_, id, &chars);

        if (status == ACAMERA_OK && chars != nullptr) {
            ACameraMetadata_const_entry entry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &entry);
            int facing = entry.data.u8[0];

            cameras.push_back({id, facing});
            ACameraMetadata_free(chars);
        }
    }

    ACameraManager_deleteCameraIdList(cameraIdList);
    return cameras;
}

} // namespace nothingraw
