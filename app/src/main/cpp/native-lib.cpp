#include <jni.h>
#include <string>
#include <vector>
#include <android/native_window_jni.h>
#include "camera_manager.h"
#include "camera_engine.h"

static nothingraw::CameraManager* gCameraManager = nullptr;
static nothingraw::CameraEngine* gCameraEngine = nullptr;

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_kazenoko_nothingraw_MainActivity_getCameraList(
        JNIEnv* env,
        jobject /* this */) {
    if (!gCameraManager) {
        gCameraManager = new nothingraw::CameraManager();
    }
    std::vector<nothingraw::CameraInfo> cameras = gCameraManager->GetCameraList();

    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray result = env->NewObjectArray(static_cast<jsize>(cameras.size()), stringClass, nullptr);

    for (size_t i = 0; i < cameras.size(); ++i) {
        env->SetObjectArrayElement(result, static_cast<jsize>(i), env->NewStringUTF(cameras[i].id.c_str()));
    }

    return result;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_kazenoko_nothingraw_MainActivity_openCamera(
        JNIEnv* env,
        jobject /* this */,
        jstring cameraId) {
    if (!gCameraManager) {
        gCameraManager = new nothingraw::CameraManager();
    }
    if (!gCameraEngine) {
        // Pass ACameraManager* from our CameraManager wrapper
        // Note: We need a way to get the internal pointer.
        // For simplicity in this JNI, I'll modify camera_manager.h to expose it or just create a local one.
        gCameraEngine = new nothingraw::CameraEngine(ACameraManager_create());
    }

    const char* id = env->GetStringUTFChars(cameraId, nullptr);
    camera_status_t status = gCameraEngine->OpenCamera(id);
    env->ReleaseStringUTFChars(cameraId, id);

    return static_cast<jint>(status);
}

extern "C" JNIEXPORT void JNICALL
Java_com_kazenoko_nothingraw_MainActivity_startPreview(
        JNIEnv* env,
        jobject /* this */,
        jobject surface) {
    if (gCameraEngine && surface) {
        ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
        gCameraEngine->StartPreview(window);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_kazenoko_nothingraw_MainActivity_stopCamera(
        JNIEnv* env,
        jobject /* this */) {
    if (gCameraEngine) {
        gCameraEngine->CloseCamera();
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kazenoko_nothingraw_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Nothing RAW Engine v0.2-alpha";
    return env->NewStringUTF(hello.c_str());
}
