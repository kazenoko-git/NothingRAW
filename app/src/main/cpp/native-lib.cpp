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

extern "C" JNIEXPORT void JNICALL
Java_com_kazenoko_nothingraw_MainActivity_openCamera(
        JNIEnv* env,
        jobject /* this */,
        jstring cameraId) {
    if (!gCameraEngine) {
        gCameraEngine = new nothingraw::CameraEngine(ACameraManager_create());
    }

    const char* id = env->GetStringUTFChars(cameraId, nullptr);
    gCameraEngine->OpenCamera(id);
    env->ReleaseStringUTFChars(cameraId, id);
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

extern "C" JNIEXPORT void JNICALL
Java_com_kazenoko_nothingraw_MainActivity_setZoom(
        JNIEnv* env,
        jobject /* this */,
        jfloat ratio) {
    if (gCameraEngine) {
        gCameraEngine->SetZoom(ratio);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_kazenoko_nothingraw_MainActivity_setPhysicalLens(
        JNIEnv* env,
        jobject /* this */,
        jstring physicalId) {
    if (gCameraEngine) {
        const char* id = env->GetStringUTFChars(physicalId, nullptr);
        gCameraEngine->SetPhysicalLens(id);
        env->ReleaseStringUTFChars(physicalId, id);
    }
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_kazenoko_nothingraw_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Nothing RAW Engine v0.4-reverted";
    return env->NewStringUTF(hello.c_str());
}
