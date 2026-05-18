#include "camera_manager.h"
#include <android/log.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

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

    for (int i = 0; i <= 5; ++i) {
        std::string testId = std::to_string(i);
        ACameraMetadata* chars = nullptr;
        camera_status_t status = ACameraManager_getCameraIdList(cameraManager_, nullptr); // Just to ensure manager is ready
        status = ACameraManager_getCameraCharacteristics(cameraManager_, testId.c_str(), &chars);

        if (status == ACAMERA_OK && chars != nullptr) {
            ACameraMetadata_const_entry facingEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &facingEntry);
            int facing = facingEntry.data.u8[0];

            ACameraMetadata_const_entry focalEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, &focalEntry);
            float focalLength = focalEntry.data.f[0];

            // Add Pixel Array Size to distinguish sensors definitively
            ACameraMetadata_const_entry pixelEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE, &pixelEntry);
            int32_t w = pixelEntry.data.i32[0];
            int32_t h = pixelEntry.data.i32[1];

            // Get Sensor Orientation
            ACameraMetadata_const_entry orientEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_ORIENTATION, &orientEntry);
            int orientation = orientEntry.data.i32[0];

            std::stringstream infoStream;
            infoStream << testId << " | " << facing << " | f=" << std::fixed << std::setprecision(2) << focalLength
                       << "mm | Res: " << w << "x" << h << " | Orient: " << orientation;

            cameras.push_back({infoStream.str(), facing});
            ACameraMetadata_free(chars);
        }
    }

    return cameras;
}

} // namespace nothingraw
