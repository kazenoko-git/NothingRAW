#ifndef NOTHINGRAW_CAMERA_MANAGER_H
#define NOTHINGRAW_CAMERA_MANAGER_H

#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraDevice.h>
#include <vector>
#include <string>

namespace nothingraw {

struct CameraInfo {
    std::string id;
    int facing; // 0 for front, 1 for back, 2 for external
};

class CameraManager {
public:
    CameraManager();
    ~CameraManager();

    std::vector<CameraInfo> GetCameraList();

private:
    ACameraManager* cameraManager_;
};

} // namespace nothingraw

#endif // NOTHINGRAW_CAMERA_MANAGER_H
