// seek_audio_aec.cc
#include "modules/audio_processing/seek_audio_aec.h"

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "common_audio/include/audio_util.h"

#include <dlfcn.h>  // dlopen, dlsym, dlclose

#define SEEKAUDIO_SAT(a, b, c)         (b > a ? a : b < c ? c : b)

namespace webrtc {

SeekAudioAec::SeekAudioAec() {
  LOGI("SeekAudioAec constructor called");
  
  if (!LoadLibrary()) {
    LOGE("Failed to load SeekAudio AEC library");
    return;
  }

  if (!aec_create_) {
    LOGE("AEC create function not available");
    return;
  }

  aec_handle_ = aec_create_();
  if (!aec_handle_) {
    LOGE("Failed to create AEC handle");
    return;
  }

  is_initialized_ = false;
  is_log_opened = false;
  LOGI("AEC handle created successfully");
  LOGI("SeekAudioAec created successfully");
}

SeekAudioAec::~SeekAudioAec() {
  LOGI("SeekAudioAec destructor called");
  
  if (aec_handle_ && aec_free_) {
    aec_free_(aec_handle_);
    aec_handle_ = nullptr;
  }
  
  UnloadLibrary();
  LOGI("SeekAudioAec destroyed");
}

bool SeekAudioAec::LoadLibrary() {
#ifdef __ANDROID__
  const char* library_path = "libseekaudio_aec.so";  // Android库路径
#else
  const char* library_path = "libseekaudio_aec.so";  // Linux库路径
#endif

  library_handle_ = dlopen(library_path, RTLD_LAZY);
  if (!library_handle_) {
    LOGE("Failed to load library: %s, error: %s", library_path, dlerror());
    return false;
  }

  LOGI("Successfully loaded library: %s", library_path);

  // 加载所有需要的函数
  aec_create_ = reinterpret_cast<AEC_CreateFunc>(GetFunctionPointer("SeekAudioAEC_Create"));
  aec_free_ = reinterpret_cast<AEC_FreeFunc>(GetFunctionPointer("SeekAudioAEC_Free"));
  aec_open_log_ = reinterpret_cast<AEC_OpenLogFunc>(GetFunctionPointer("SeekAudioAEC_OpenLog"));
  aec_init_ = reinterpret_cast<AEC_InitFunc>(GetFunctionPointer("SeekAudioAEC_Init"));
  aec_process_ = reinterpret_cast<AEC_ProcessFunc>(GetFunctionPointer("SeekAudioAEC_Process"));
  aec_buffer_farend_ = reinterpret_cast<AEC_BufferFarendFunc>(GetFunctionPointer("SeekAudioAEC_buffer_farend"));
  aec_agc_compensate_ = reinterpret_cast<AEC_AGC_CompensateFunc>(GetFunctionPointer("SeekAudioAEC_AGC_Compensate"));
  aec_get_howling_status_ = reinterpret_cast<AEC_GetHowlingStatusFunc>(GetFunctionPointer("SeekAudioAEC_GetHowlingStatus"));
  aec_set_power_howl_ = reinterpret_cast<AEC_SetPowerFunc>(GetFunctionPointer("SeekAudioAEC_Set_AI_Engine_Power_For_Howl"));
  aec_set_power_echo_ = reinterpret_cast<AEC_SetPowerFunc>(GetFunctionPointer("SeekAudioAEC_Set_AI_Engine_Power_For_Echo"));
  // 检查必需函数是否加载成功
  if (!aec_create_ || !aec_free_ || !aec_init_ || !aec_process_ || !aec_buffer_farend_) {
    LOGE("Failed to load essential AEC functions");
    UnloadLibrary();
    return false;
  }

  LOGI("All AEC functions loaded successfully");
  return true;
}

void SeekAudioAec::UnloadLibrary() {
  if (library_handle_) {
    dlclose(library_handle_);
    library_handle_ = nullptr;
    
    // 清空函数指针
    aec_create_ = nullptr;
    aec_free_ = nullptr;
    aec_open_log_ = nullptr;
    aec_init_ = nullptr;
	aec_set_power_howl_ = nullptr;
    aec_process_ = nullptr;
    aec_buffer_farend_ = nullptr;
    aec_agc_compensate_ = nullptr;
    aec_get_howling_status_ = nullptr;
	aec_set_power_echo_ = nullptr;
    
    LOGI("AEC library unloaded");
  }
}

void* SeekAudioAec::GetFunctionPointer(const char* function_name) {
  if (!library_handle_) {
    LOGE("Library not loaded, cannot get function: %s", function_name);
    return nullptr;
  }

  void* func_ptr = dlsym(library_handle_, function_name);
  if (!func_ptr) {
    LOGW("Failed to load function: %s, error: %s", function_name, dlerror());
    return nullptr;
  }

  LOGI("Successfully loaded function: %s", function_name);
  return func_ptr;
}

void SeekAudioAec::SetSuppressPowerHowl(int level)
{
	LOGI("SeekAudioAec::SetSuppressPowerHowl called: level=%d", level);
	if (!library_handle_) {
		LOGE("Library not loaded, cannot initialize");
		return ;
	}

	if (!aec_set_power_howl_) {
		LOGE("aec_set_power_howl_ function not available");
		return ;
	}

	aec_set_power_howl_(aec_handle_, level);

}

void SeekAudioAec::SetSuppressPowerEcho(int level)
{
	LOGI("SeekAudioAec::SetSuppressPowerEcho called: level=%d", level);
	if (!library_handle_) {
		LOGE("Library not loaded, cannot initialize");
		return;
	}

	if (!aec_set_power_echo_) {
		LOGE("aec_set_power_echo_ function not available");
		return;
	}

	aec_set_power_echo_(aec_handle_, level);

}

bool SeekAudioAec::Initialize(int sample_rate_hz) {
  if (is_initialized_)
        return true;
  LOGI("SeekAudioAec::Initialize called: sample_rate=%d", sample_rate_hz);
  
  RTC_DCHECK_GT(sample_rate_hz, 0);
  
  if (!library_handle_) {
    LOGE("Library not loaded, cannot initialize");
    return false;
  }
  

  if (sample_rate_hz != 16000) {
    LOGE("SeekAudioAEC only supports 16kHz sample rates, got %d", sample_rate_hz);
    return false;
  }
  
  sample_rate_hz_ = sample_rate_hz;

 
  if (!aec_init_) {
    LOGE("AEC init function not available");
    return false;
  }
  
  if (aec_init_(aec_handle_, sample_rate_hz_) != 0) {
    LOGE("Failed to initialize SeekAudioAEC");
    if (aec_free_) {
      aec_free_(aec_handle_);
      aec_handle_ = nullptr;
    }
    return false;
  }
  
  is_initialized_ = true;
  LOGI("SeekAudioAEC initialized successfully: sample_rate=%d",sample_rate_hz_);
  
  return true;
}

void SeekAudioAec::ProcessCaptureAudio(float* const* data, int samples_per_channel) {
  if (!is_initialized_ || !aec_handle_ || !aec_process_) {
    LOGW("AEC not ready for processing: initialized=%d, handle=%p, process_func=%p",
         is_initialized_, aec_handle_, aec_process_);
    return;
  }
  
  if (samples_per_channel <= 0) {
    LOGW("Invalid samples per channel: %d", samples_per_channel);
    return;
  }

  int i = 0;
  float* audioframe = data[0];
  short near_frame[160];
  short out_frame[160];

  for (i = 0; i < 160; i++)
  {
	  near_frame[i] = SEEKAUDIO_SAT(32767, audioframe[i], -32768);
	  out_frame[i] = 0;
  }
  //LOGI("Processing capture audio: %d samples", samples_per_channel);
  
  int ret = aec_process_(aec_handle_, near_frame, out_frame, samples_per_channel);
  if (ret != 0) {
    LOGW("AEC process returned error: %d", ret);
  }
  
  for (i = 0; i < 160; i++)
  {
	  audioframe[i] = (float)out_frame[i];
  }

  //LOGI("Capture audio processing completed");
}

void SeekAudioAec::BufferFarend(float* const* farend, int samples_per_channel) {
  if (!is_initialized_ || !aec_handle_ || !aec_buffer_farend_) {
    LOGW("AEC not ready for buffering farend: initialized=%d, handle=%p, buffer_func=%p",
         is_initialized_, aec_handle_, aec_buffer_farend_);
    return;
  }
  
  if (samples_per_channel <= 0) {
    LOGW("Invalid samples per channel: %d", samples_per_channel);
    return;
  }
  
  //LOGI("Buffering farend audio: %d samples", samples_per_channel);
  int i = 0;
  float* dataTemp = farend[0];
  short far_frame[160];
  for (i = 0; i < 160; i++)
  {
	  far_frame[i] = SEEKAUDIO_SAT(32767, dataTemp[i], -32768);
  }

  int ret = aec_buffer_farend_(aec_handle_, far_frame, samples_per_channel);
  if (ret != 0) {
    LOGW("AEC buffer farend returned error: %d", ret);
  }
}

void SeekAudioAec::ProcessAGCCompensate(float* const* agcIn, float* const* agcOut, float* const* data, int samples_per_channel){
  if (!is_initialized_ || !aec_handle_ || !aec_agc_compensate_) {
    LOGW("AEC not ready for AGC compensation: initialized=%d, handle=%p, agc_func=%p",
         is_initialized_, aec_handle_, aec_agc_compensate_);
    return;
  }
  
  if (samples_per_channel <= 0) {
    LOGW("Invalid samples per channel: %d", samples_per_channel);
    return;
  }
  
  int i = 0;
  float* dataTemp = data[0];
  float* agcInTemp = agcIn[0];
  float* agcOutTemp = agcOut[0];
  short agc_in[160];
  short agc_out[160];
  short in_frame[160];
  short out_frame[160];
  for (i = 0; i < 160; i++)
  {
	  agc_in[i] = SEEKAUDIO_SAT(32767, agcInTemp[i], -32768);
	  agc_out[i] = SEEKAUDIO_SAT(32767, agcOutTemp[i], -32768);
	  in_frame[i] = SEEKAUDIO_SAT(32767, dataTemp[i], -32768);
  }
  
  aec_agc_compensate_(aec_handle_, agc_in, agc_out, in_frame, out_frame);

  for (i = 0; i < 160; i++)
  {
	  dataTemp[i] = (float)out_frame[i];
  }

}

int SeekAudioAec::ProcessOpenLog(const char* folder_path) {
  int ret = 0;

  if (is_log_opened)
      return ret;

  if (!aec_handle_ || !aec_open_log_) {
    LOGE("AEC ProcessOpenLog function failed");
    return -1;
  }

  ret = aec_open_log_(aec_handle_, folder_path);

  if (ret != 0) {
    LOGE("AEC ProcessOpenLog function failed, ret: %d", ret);
    return ret;
  }
  is_log_opened = true;
  return ret;
}

float SeekAudioAec::GetHowlingProbability() const {
  if (!is_initialized_ || !aec_handle_ || !aec_get_howling_status_) {
    LOGW("Cannot get howling probability: initialized=%d, handle=%p, func=%p",
         is_initialized_, aec_handle_, aec_get_howling_status_);
    return 0.0f;
  }
  
  float probability = aec_get_howling_status_(aec_handle_);
  //LOGI("Howling probability: %.3f", probability);
  return probability;
}

}  // namespace webrtc
