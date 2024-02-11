export WEBRTC_AEC3_SRC += \
    common_audio/resampler/sinc_resampler_avx2.o \
    modules/audio_processing/aec3/adaptive_fir_filter_erl_avx2.o \
    modules/audio_processing/aec3/adaptive_fir_filter_avx2.o \
    modules/audio_processing/aec3/fft_data_avx2.o \
    modules/audio_processing/aec3/matched_filter_avx2.o \
    modules/audio_processing/aec3/vector_math_avx2.o \
    modules/audio_processing/agc2/rnn_vad/vector_math_avx2.o
    WEBRTC_AEC3_OTHER_CFLAGS += -mfma
    WEBRTC_AEC3_OTHER_CXXFLAGS += -mfma