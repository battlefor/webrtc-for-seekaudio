// seek_audio_afc.h
#ifndef MODULES_AUDIO_PROCESSING_SEEK_AUDIO_AFC_H_
#define MODULES_AUDIO_PROCESSING_SEEK_AUDIO_AFC_H_

#include <memory>
#include <vector>
#include <string>

#include "api/audio/audio_processing.h"

// Android日志支持
#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "SEEKAUDIO"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#else
#define LOGI(...) printf("[INFO] " __VA_ARGS__)
#define LOGW(...) printf("[WARN] " __VA_ARGS__)
#define LOGE(...) printf("[ERROR] " __VA_ARGS__)
#endif

namespace webrtc {

class SeekAudioAfc {
 public:
  explicit SeekAudioAfc();
  ~SeekAudioAfc();


  bool Initialize(int sample_rate_hz, int num_channels);
  void SetSuppressPower(int level);

  void ProcessCaptureAudio(float* const* data, int samples_per_channel);

  void ProcessAGCCompensate(float* const* agc_in, float* const* agc_out, float* const* data, int samples_per_channel);
  

  int  ProcessOpenLog(const char *folderPath);
  

  float GetHowlingProbability() const;

 private:
  // 动态加载的函数指针类型定义
  typedef void* (*AFC_CreateFunc)();
  typedef void (*AFC_FreeFunc)(void*);
  typedef int (*AFC_OpenLogFunc)(void*, const char*);
  typedef int (*AFC_InitFunc)(void*, int);
  typedef void(*AFC_SetPowerFunc)(void*, int);
  typedef void (*AFC_ProcessFunc)(void*, const float*, float*);
  typedef void (*AFC_ProcessShortFunc)(void*, const short*, short*);
  typedef void (*AFC_AGC_CompensateFunc)(void*, const float*, const float*, const float*, float*);
  typedef void (*AFC_AGC_CompensateShortFunc)(void*, const short*, const short*, const short*, short*);
  typedef float (*AFC_GetHowlingStatusFunc)(void*);

  // 函数指针声明
  AFC_CreateFunc afc_create_ = nullptr;
  AFC_FreeFunc afc_free_ = nullptr;
  AFC_OpenLogFunc afc_open_log_ = nullptr;
  AFC_InitFunc afc_init_ = nullptr;
  AFC_SetPowerFunc afc_set_power_ = nullptr;
  AFC_ProcessFunc afc_process_ = nullptr;
  AFC_ProcessShortFunc afc_process_short_ = nullptr;
  AFC_AGC_CompensateFunc afc_agc_compensate_ = nullptr;
  AFC_AGC_CompensateShortFunc afc_agc_compensate_short_ = nullptr;
  AFC_GetHowlingStatusFunc afc_get_howling_status_ = nullptr;

  // 动态库句柄
  void* library_handle_ = nullptr;
  
  bool is_initialized_ = false;
  int sample_rate_hz_ = 0;
  
  void* afc_handle_ = nullptr;  // 使用void*代替具体的AFCHandle*
  
  // 音频缓冲区
  std::vector<float> processing_buffer_;
  std::vector<const float*> channel_ptrs_;
  
  // 动态加载库
  bool LoadLibrary();
  void UnloadLibrary();
  void* GetFunctionPointer(const char* function_name);
  
};

}  // namespace webrtc

#endif  // MODULES_AUDIO_PROCESSING_SEEK_AUDIO_AFC_H_