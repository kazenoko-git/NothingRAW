#include "camera_manager.h"
#include <android/log.h>
#include <vector>
#include <string>

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

    // 1. Standard Discovery
    ACameraIdList* cameraIdList = nullptr;
    camera_status_t status = ACameraManager_getCameraIdList(cameraManager_, &cameraIdList);

    if (status == ACAMERA_OK && cameraIdList != nullptr) {
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
    }

    // 2. Brute-Force Discovery (Testing IDs 2-10)
    // On some devices, physical IDs are not in the list but can still be queried directly.
    for (int i = 2; i <= 10; ++i) {
        std::string testId = std::to_string(i);
        ACameraMetadata* chars = nullptr;
        status = ACameraManager_getCameraCharacteristics(cameraManager_, testId.c_str(), &chars);

        if (status == ACAMERA_OK && chars != nullptr) {
            ACameraMetadata_const_entry entry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &entry);
            int facing = entry.data.u8[0];

            // Check if we already have this ID
            bool exists = false;
            for (const auto& cam : cameras) {
                if (cam.id == testId) {
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                cameras.push_back({testId + " (Unmasked)", facing});
            }
            ACameraMetadata_free(chars);
        }
    }

    return cameras;
}

} // namespace nothingraw
