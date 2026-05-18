#include <jni.h>
#include <string>
#include <vector>
#include "camera_manager.h"

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_kazenoko_nothingraw_MainActivity_getCameraList(
        JNIEnv* env,
        jobject /* this */) {
    nothingraw::CameraManager cameraManager;
    std::vector<nothingraw::CameraInfo> cameras = cameraManager.GetCameraList();

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(cameras.size(), stringClass, nullptr);

    for (size_t i = 0; i < cameras.size(); ++i) {
        std::string info = cameras[i].id + ":" + std::to_string(cameras[i].facing);
        env->SetObjectArrayElement(result, i, env->NewStringUTF(info.c_str()));
    }

    return result;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kazenoko_nothingraw_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Nothing RAW Engine Initialized";
    return env->NewStringUTF(hello.c_str());
}
