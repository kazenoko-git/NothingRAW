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

    // Probing IDs 0-5 aggressively
    for (int i = 0; i <= 5; ++i) {
        std::string testId = std::to_string(i);
        ACameraMetadata* chars = nullptr;
        camera_status_t status = ACameraManager_getCameraCharacteristics(cameraManager_, testId.c_str(), &chars);

        if (status == ACAMERA_OK && chars != nullptr) {
            ACameraMetadata_const_entry facingEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_FACING, &facingEntry);
            int facing = facingEntry.data.u8[0];

            // Query Focal Length to identify lens type (Ultra-wide vs Tele)
            ACameraMetadata_const_entry focalEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_LENS_INFO_AVAILABLE_FOCAL_LENGTHS, &focalEntry);
            float focalLength = focalEntry.data.f[0];

            // Query Sensor Size to differentiate 50MP vs 8MP
            ACameraMetadata_const_entry sensorSizeEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_SENSOR_INFO_PHYSICAL_SIZE, &sensorSizeEntry);
            float width = sensorSizeEntry.data.f[0];
            float height = sensorSizeEntry.data.f[1];

            // Query FPS ranges to see what's officially allowed
            ACameraMetadata_const_entry fpsEntry;
            ACameraMetadata_getConstEntry(chars, ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES, &fpsEntry);

            std::stringstream fpsStream;
            fpsStream << "FPS: ";
            for (uint32_t j = 0; j < fpsEntry.count; j += 2) {
                fpsStream << "[" << (int)fpsEntry.data.i32[j] << "," << (int)fpsEntry.data.i32[j+1] << "] ";
            }

            std::stringstream infoStream;
            infoStream << testId << " | " << facing << " | f=" << std::fixed << std::setprecision(2) << focalLength
                       << "mm | sensor=" << width << "x" << height << "mm | " << fpsStream.str();

            cameras.push_back({infoStream.str(), facing});
            ACameraMetadata_free(chars);
        }
    }

    return cameras;
}

} // namespace nothingraw
