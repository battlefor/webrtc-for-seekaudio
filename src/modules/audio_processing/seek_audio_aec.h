// seek_audio_aec.h
#ifndef MODULES_AUDIO_PROCESSING_SEEK_AUDIO_AEC_H_
#define MODULES_AUDIO_PROCESSING_SEEK_AUDIO_AEC_H_

#include <memory>
#include <vector>
#include <string>

#include "api/audio/audio_processing.h"

// Android日志支持
#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "SEEKAUDIO_AEC"
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#else
#define LOGI(...) printf("[INFO] " __VA_ARGS__)
#define LOGW(...) printf("[WARN] " __VA_ARGS__)
#define LOGE(...) printf("[ERROR] " __VA_ARGS__)
#endif

namespace webrtc {


class SeekAudioAec {
 public:
  explicit SeekAudioAec();
  ~SeekAudioAec();

  // 初始化AEC
  bool Initialize(int sample_rate_hz);

  void SetSuppressPowerHowl(int level);
  void SetSuppressPowerEcho(int level);
  
  // 处理近端音频（包含回音消除）
  void ProcessCaptureAudio(float* const* data, int samples_per_channel);
  
  // 缓冲远端音频（用于回音消除参考）
  void BufferFarend(float* const* farend, int samples_per_channel);
  
  // AGC补偿处理
  void ProcessAGCCompensate(float* const* agcIn, float* const* agcOut,float* const* data, int samples_per_channel);
  
  // 打开日志
  int ProcessOpenLog(const char* folder_path);
  
  // 获取啸叫状态概率
  float GetHowlingProbability() const;

 private:
  // 动态加载的函数指针类型定义
  typedef void* (*AEC_CreateFunc)();
  typedef void (*AEC_FreeFunc)(void*);
  typedef int (*AEC_OpenLogFunc)(void*, const char*);
  typedef int (*AEC_InitFunc)(void*, int);
  typedef void(*AEC_SetPowerFunc)(void*, int); 
  typedef int (*AEC_ProcessFunc)(void*, const short*, short*, int);
  typedef int (*AEC_BufferFarendFunc)(void*, const short*, int);
  typedef void (*AEC_AGC_CompensateFunc)(void*, const short*, const short*, const short*, short*);
  typedef float (*AEC_GetHowlingStatusFunc)(void*);

  // 函数指针声明
  AEC_CreateFunc aec_create_ = nullptr;
  AEC_FreeFunc aec_free_ = nullptr;
  AEC_OpenLogFunc aec_open_log_ = nullptr;
  AEC_InitFunc aec_init_ = nullptr;
  AEC_SetPowerFunc aec_set_power_howl_= nullptr;
  AEC_SetPowerFunc aec_set_power_echo_ = nullptr;
  AEC_ProcessFunc aec_process_ = nullptr;
  AEC_BufferFarendFunc aec_buffer_farend_ = nullptr;
  AEC_AGC_CompensateFunc aec_agc_compensate_ = nullptr;
  AEC_GetHowlingStatusFunc aec_get_howling_status_ = nullptr;


  void* library_handle_ = nullptr;
  
  bool is_initialized_ = false;
  int sample_rate_hz_ = 0;
  
  void* aec_handle_ = nullptr; 
  

  bool LoadLibrary();
  void UnloadLibrary();
  void* GetFunctionPointer(const char* function_name);
};

}  // namespace webrtc

#endif  