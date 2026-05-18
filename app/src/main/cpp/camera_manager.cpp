#include "camera_manager.h"
#include <android/log.h>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

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

    for (int i = 0; i <= 10; ++i) {
        std::string testId = std::to_string(i);
        ACameraMetadata* chars = nullptr;
        camera_status_t status = ACameraManager_getCameraCharacteristics(cameraManager_, testId.c_str(), &chars);

        if (status == ACAMERA_OK && chars != nullptr) {
            ACameraMetadata_const_entry entry;

            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &entry);
            int facing = entry.data.u8[0];

            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, &entry);
            float focalLength = entry.data.f[0];

            ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_INFO_PHYSICAL_SIZE, &entry);
            float sensorW = entry.data.f[0];
            float sensorH = entry.data.f[1];

            ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_INFO_PIXEL_ARRAY_SIZE, &entry);
            int32_t pixelW = entry.data.i32[0];
            int32_t pixelH = entry.data.i32[1];

            float hfov = 2.0f * atanf(sensorW / (2.0f * focalLength)) * (180.0f / M_PI);

            std::stringstream infoStream;
            infoStream << testId << " | f=" << std::fixed << std::setprecision(2) << focalLength << "mm"
                       << " | FOV: " << std::setprecision(1) << hfov << "°"
                       << " | Res: " << pixelW << "x" << pixelH;

            cameras.push_back({infoStream.str(), facing});
            ACameraMetadata_free(chars);
        }
    }

    return cameras;
}

} // namespace nothingraw
