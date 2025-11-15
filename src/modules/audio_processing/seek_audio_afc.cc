// seek_audio_afc.cc
#include "modules/audio_processing/seek_audio_afc.h"

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "common_audio/include/audio_util.h"

#include <dlfcn.h>  // dlopen, dlsym, dlclose
#define SEEKAUDIO_SAT(a, b, c)         (b > a ? a : b < c ? c : b)

namespace webrtc {

SeekAudioAfc::SeekAudioAfc()
{
  LOGI("SeekAudioAfc constructor called");
  
  // 尝试加载库
  if (!LoadLibrary()) {
    LOGE("Failed to load SeekAudio AFC library");
    return;
  }

  // 创建AFC句柄
  if (!afc_create_) {
      LOGE("AFC create function not available");
      return;
  }

  afc_handle_ = afc_create_();
  if (!afc_handle_) {
      LOGE("Failed to create AFC handle");
      return;
  }
  is_initialized_ = false;
  is_log_opened = false;
  LOGI("SeekAudioAfc created successfully");
}

SeekAudioAfc::~SeekAudioAfc() {
  LOGI("SeekAudioAfc destructor called");
  
  if (afc_handle_ && afc_free_) {
    afc_free_(afc_handle_);
    afc_handle_ = nullptr;
  }
  
  UnloadLibrary();
  LOGI("SeekAudioAfc destroyed");
}

bool SeekAudioAfc::LoadLibrary() {
#ifdef __ANDROID__
  const char* library_path = "libseekaudio_afc.so";  // Android库路径
#else
  const char* library_path = "libseekaudio_afc.so";  // Linux库路径
#endif

  library_handle_ = dlopen(library_path, RTLD_LAZY);
  if (!library_handle_) {
    LOGE("Failed to load library: %s, error: %s", library_path, dlerror());
    return false;
  }

  LOGI("Successfully loaded library: %s", library_path);

  // 加载所有需要的函数
  afc_create_ = reinterpret_cast<AFC_CreateFunc>(GetFunctionPointer("SeekAudioAFC_Create"));
  afc_free_ = reinterpret_cast<AFC_FreeFunc>(GetFunctionPointer("SeekAudioAFC_Free"));
  afc_open_log_ = reinterpret_cast<AFC_OpenLogFunc>(GetFunctionPointer("SeekAudioAFC_OpenLog"));
  afc_init_ = reinterpret_cast<AFC_InitFunc>(GetFunctionPointer("SeekAudioAFC_Init"));
  afc_process_ = reinterpret_cast<AFC_ProcessFunc>(GetFunctionPointer("SeekAudioAFC_Process"));
  afc_agc_compensate_ = reinterpret_cast<AFC_AGC_CompensateFunc>(GetFunctionPointer("SeekAudioAFC_AGC_Compensate"));
  afc_get_howling_status_ = reinterpret_cast<AFC_GetHowlingStatusFunc>(GetFunctionPointer("SeekAudioAFC_GetHowlingStatus"));
  afc_set_power_ = reinterpret_cast<AFC_SetPowerFunc>(GetFunctionPointer("SeekAudioAFC_Set_AI_Engine_Power"));

  // 检查必需函数是否加载成功
  if (!afc_create_ || !afc_free_ || !afc_init_ || !afc_process_) {
    LOGE("Failed to load essential AFC functions");
    UnloadLibrary();
    return false;
  }

  LOGI("All AFC functions loaded successfully");
  return true;
}

void SeekAudioAfc::UnloadLibrary() {
  if (library_handle_) {
    dlclose(library_handle_);
    library_handle_ = nullptr;
    
    // 清空函数指针
    afc_create_ = nullptr;
    afc_free_ = nullptr;
    afc_open_log_ = nullptr;
    afc_init_ = nullptr;
	afc_set_power_ = nullptr;
    afc_process_ = nullptr;
    afc_agc_compensate_ = nullptr;
    afc_get_howling_status_ = nullptr;
    
    LOGI("AFC library unloaded");
  }
}

void* SeekAudioAfc::GetFunctionPointer(const char* function_name) {
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

void SeekAudioAfc::SetSuppressPower(int level)
{
	LOGI("SeekAudioAfc::SetSuppressPower called: level=%d", level);
	if (!library_handle_) {
		LOGE("Library not loaded, cannot initialize");
		return;
	}

	if (!afc_set_power_) {
		LOGE("afc_set_power_ function not available");
		return;
	}

	afc_set_power_(afc_handle_, level);

}

bool SeekAudioAfc::Initialize(int sample_rate_hz, int num_channels) {
  if (is_initialized_)
		return true;
  LOGI("SeekAudioAfc::Initialize called: sample_rate=%d, channels=%d", 
       sample_rate_hz, num_channels);
  
  RTC_DCHECK_GT(sample_rate_hz, 0);
  RTC_DCHECK_GT(num_channels, 0);
  
  if (!library_handle_) {
    LOGE("Library not loaded, cannot initialize");
    return false;
  }
  
  if (sample_rate_hz != 16000) {
    LOGE("SeekAudioAFC only supports 16kHz sample rate, got %d", sample_rate_hz);
    return false;
  }
  
  if (num_channels != 1) {
	  LOGE("SeekAudioAFC only supports mono channel, num_channels %d", num_channels);
	  return false;
  }

  sample_rate_hz_ = sample_rate_hz;


  // 初始化AFC
  if (!afc_init_) {
    LOGE("AFC init function not available");
    return false;
  }
  
  if (afc_init_(afc_handle_, sample_rate_hz_) != 0) {
    LOGE("Failed to initialize SeekAudioAFC");
    if (afc_free_) {
      afc_free_(afc_handle_);
      afc_handle_ = nullptr;
    }
    return false;
  }
  
  is_initialized_ = true;
  LOGI("SeekAudioAFC initialized successfully: sample_rate=%d",
       sample_rate_hz_);
  
  return true;
}

int  SeekAudioAfc::ProcessOpenLog(const char *folderPath){
	int ret = 0;
	if (is_log_opened)
		return ret;
	if (!afc_handle_ || !afc_open_log_) {
		LOGE("AFC ProcessOpenLog function failed");
		return -1;
	}

	ret = afc_open_log_(afc_handle_, folderPath);

	if (ret != 0)
	{
		LOGE("AFC ProcessOpenLog function failed,ret:%d", ret);
		return ret;
	}

	is_log_opened = true;
	return ret;
}

void SeekAudioAfc::ProcessAGCCompensate(float* const* agc_in, float* const* agc_out, float* const* data, int samples_per_channel)
{
	if (!is_initialized_ || !afc_handle_ || !afc_agc_compensate_) {
		LOGW("AFC not ready for processing: initialized=%d, handle=%p, process_func=%p",
			is_initialized_, afc_handle_, afc_agc_compensate_);
		return;
	}

	if (samples_per_channel != 160) {
		LOGW("AFC requires 10ms frames (160 samples), got %d samples", samples_per_channel);
		return;
	}

	int i = 0;
	float* tempframe = data[0];
	float* agcframeIn = agc_in[0];
	float* agcframeOut = agc_out[0];

	short input_frame[160];
	short output_frame[160];
	short agc_in_frame[160];
	short agc_out_frame[160];

	for (i = 0; i < 160; i++)
	{
		input_frame[i] = SEEKAUDIO_SAT(32767, tempframe[i], -32768);
		agc_in_frame[i] = SEEKAUDIO_SAT(32767, agcframeIn[i], -32768);
		agc_out_frame[i] = SEEKAUDIO_SAT(32767, agcframeOut[i], -32768);
		output_frame[i] = 0;
	}

	afc_agc_compensate_(afc_handle_, agc_in_frame, agc_out_frame, input_frame, output_frame);


	for (i = 0; i < 160; i++)
	{
		tempframe[i] = (float)output_frame[i];
	}

}

void SeekAudioAfc::ProcessCaptureAudio(float* const* data, int samples_per_channel) {
  if (!is_initialized_ || !afc_handle_ || !afc_process_) {
    LOGW("AFC not ready for processing: initialized=%d, handle=%p, process_func=%p",
         is_initialized_, afc_handle_, afc_process_);
    return;
  }
  
  if (samples_per_channel != 160 ) {
    LOGW("AFC requires 10ms frames (160 samples), got %d samples", samples_per_channel);
    return;
  }
  
  //LOGI("Processing capture audio: %d samples",samples_per_channel);
  int i = 0;
  float* temp_frame = data[0];
  short input_frame[160];
  short output_frame[160];
  for (i = 0; i < 160; i++)
  {
	  input_frame[i] = SEEKAUDIO_SAT(32767, temp_frame[i], -32768);
	  output_frame[i] = 0;
  }

  afc_process_(afc_handle_, input_frame, output_frame);

  for (i = 0; i < 160; i++)
  {
	  temp_frame[i] =(float)output_frame[i];
  }

}

float SeekAudioAfc::GetHowlingProbability() const {
  if (!is_initialized_ || !afc_handle_ || !afc_get_howling_status_) {
    LOGW("Cannot get howling probability: initialized=%d, handle=%p, func=%p",
         is_initialized_, afc_handle_, afc_get_howling_status_);
    return 0.0f;
  }
  
  float probability = afc_get_howling_status_(afc_handle_);
  //LOGI("Howling probability: %.3f", probability);
  return probability;
}

}  // namespace webrtc
