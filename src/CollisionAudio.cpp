#include "CollisionAudio.h"

// Only playback-device support is needed: no file decoder, sound assets, engine,
// capture, or resource-manager thread. Backends load operating-system APIs.
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MA_NO_NULL
#define MA_NO_JACK
#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace pool {

struct CollisionAudio::Impl {
    CollisionSoundMixer mixer;
    ma_device device{};
    bool available=false;

    static void callback(ma_device* device,void* output,const void*,ma_uint32 frames) {
        auto* self=static_cast<Impl*>(device->pUserData);
        self->mixer.render(static_cast<float*>(output),frames);
    }
    ~Impl() { if(available) ma_device_uninit(&device); }
};

CollisionAudio::CollisionAudio():impl_(std::make_unique<Impl>()) {}
CollisionAudio::~CollisionAudio()=default;

bool CollisionAudio::initialize(std::string& error) {
    error.clear();
    if(impl_->available) return true;
    ma_device_config config=ma_device_config_init(ma_device_type_playback);
    config.playback.format=ma_format_f32;
    config.playback.channels=2;
    config.sampleRate=CollisionSoundMixer::sampleRate;
    config.periodSizeInFrames=256;
    config.dataCallback=Impl::callback;
    config.pUserData=impl_.get();
    ma_result result;
#if defined(__linux__)
    // PulseAudio is also how WSLg forwards Linux sound to Windows speakers.
    const ma_backend backends[]={ma_backend_pulseaudio,ma_backend_alsa};
    result=ma_device_init_ex(backends,2,nullptr,&config,&impl_->device);
#else
    result=ma_device_init(nullptr,&config,&impl_->device);
#endif
    if(result!=MA_SUCCESS) {
        error=std::string("Collision sound unavailable: ")+ma_result_description(result);
        return false;
    }
    result=ma_device_start(&impl_->device);
    if(result!=MA_SUCCESS) {
        ma_device_uninit(&impl_->device);
        error=std::string("Could not start collision sound: ")+ma_result_description(result);
        return false;
    }
    impl_->available=true;
    return true;
}

bool CollisionAudio::isAvailable() const noexcept { return impl_->available; }
void CollisionAudio::playImpact(double speed,bool cushion) noexcept {
    if(impl_->available) impl_->mixer.enqueueImpact(speed,cushion);
}
void CollisionAudio::setMuted(bool muted) noexcept { impl_->mixer.setMuted(muted); }

} // namespace pool
