#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace pool {

// The speed is the incoming relative speed along the contact normal (m/s).
// The small threshold suppresses numerical chatter without affecting physics.
inline float impactGain(double normalClosingSpeed, bool cushion=false) noexcept {
    if(!std::isfinite(normalClosingSpeed) || normalClosingSpeed<=0.035) return 0;
    const double strength=std::clamp((normalClosingSpeed-0.035)/2.965,0.0,1.0);
    const double gain=0.035+0.28*std::pow(strength,0.65);
    return static_cast<float>(gain*(cushion?0.8:1.0));
}

// A small hardware-independent sound generator. The simulation thread enqueues
// impacts; the audio callback alone owns/advances the voices. No allocation,
// mutex, or device call occurs in render(). Samples are interleaved stereo.
class CollisionSoundMixer {
public:
    static constexpr std::uint32_t sampleRate=48000;
    static constexpr std::size_t soundFrames=4096;
    static constexpr std::size_t maxVoices=32;

    CollisionSoundMixer() {
        constexpr double pi=3.14159265358979323846;
        std::uint32_t random=0x91e10da5u;
        for(std::size_t kind=0;kind<2;++kind) {
            float peak=0;
            for(std::size_t i=0;i<soundFrames;++i) {
                const double t=static_cast<double>(i)/sampleRate;
                random=random*1664525u+1013904223u;
                const double noise=(static_cast<double>(random>>8)/16777215.0)*2-1;
                const double attack=std::min(1.0,static_cast<double>(i)/24.0);
                const double tail=std::min(1.0,static_cast<double>(soundFrames-1-i)/150.0);
                double value;
                if(kind==0) {
                    value=.58*std::sin(2*pi*2450*t)*std::exp(-t/.010)
                         +.22*std::sin(2*pi*1250*t)*std::exp(-t/.019)
                         +.20*noise*std::exp(-t/.004);
                } else {
                    value=.65*std::sin(2*pi*420*t)*std::exp(-t/.027)
                         +.25*std::sin(2*pi*800*t)*std::exp(-t/.010)
                         +.10*noise*std::exp(-t/.006);
                }
                sounds_[kind][i]=static_cast<float>(value*attack*tail);
                peak=std::max(peak,std::abs(sounds_[kind][i]));
            }
            if(peak>0) for(auto& value:sounds_[kind]) value*=.92f/peak;
        }
    }

    // Single producer: call from the main/simulation thread only.
    void enqueueImpact(double speed,bool cushion=false) noexcept {
        const float gain=impactGain(speed,cushion);
        if(gain==0 || muted_.load(std::memory_order_relaxed)) return;
        const auto write=write_.load(std::memory_order_relaxed);
        const auto next=(write+1)%queueSize;
        if(next==read_.load(std::memory_order_acquire)) return; // Bounded overflow.
        queue_[write]={gain,static_cast<std::uint32_t>(cushion?1:0)};
        write_.store(next,std::memory_order_release);
    }

    void setMuted(bool muted) noexcept { muted_.store(muted,std::memory_order_relaxed); }

    // Single consumer: the playback thread only. Safe also for offline synthesis.
    void render(float* stereo,std::size_t frameCount) noexcept {
        if(!stereo) return;
        if(muted_.load(std::memory_order_relaxed)) {
            read_.store(write_.load(std::memory_order_acquire),std::memory_order_release);
            for(auto& voice:voices_) voice.frame=soundFrames;
            std::fill_n(stereo,frameCount*2,0.f);
            return;
        }
        auto read=read_.load(std::memory_order_relaxed);
        const auto write=write_.load(std::memory_order_acquire);
        while(read!=write) {
            startVoice(queue_[read]);
            read=(read+1)%queueSize;
        }
        read_.store(read,std::memory_order_release);
        for(std::size_t frame=0;frame<frameCount;++frame) {
            float mixed=0;
            for(auto& voice:voices_) if(voice.frame<soundFrames) {
                mixed+=sounds_[voice.kind][voice.frame++]*voice.gain;
            }
            // Soft limiting prevents clipping during a multi-ball rack impact.
            const float sample=mixed/(1.f+std::abs(mixed));
            stereo[frame*2]=sample;
            stereo[frame*2+1]=sample;
        }
    }

private:
    static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
                  "Collision audio requires lock-free 32-bit atomics");
    static_assert(std::atomic<bool>::is_always_lock_free,
                  "Collision audio requires a lock-free mute flag");
    static constexpr std::uint32_t queueSize=128;
    struct Command { float gain=0; std::uint32_t kind=0; };
    struct Voice { std::size_t frame=soundFrames; float gain=0; std::uint32_t kind=0; };
    std::array<std::array<float,soundFrames>,2> sounds_{};
    std::array<Command,queueSize> queue_{};
    std::array<Voice,maxVoices> voices_{};
    std::atomic<std::uint32_t> write_{0},read_{0};
    std::atomic<bool> muted_{false};

    void startVoice(Command command) noexcept {
        Voice* quietest=&voices_[0];
        float quietestRemaining=1e10f;
        for(auto& voice:voices_) {
            if(voice.frame>=soundFrames) { voice={0,command.gain,command.kind}; return; }
            const float remaining=voice.gain*static_cast<float>(soundFrames-voice.frame)/soundFrames;
            if(remaining<quietestRemaining) { quietest=&voice; quietestRemaining=remaining; }
        }
        if(command.gain>quietestRemaining) *quietest={0,command.gain,command.kind};
    }
};

// Device ownership stays outside physics. A missing playback device is nonfatal.
class CollisionAudio {
public:
    CollisionAudio();
    ~CollisionAudio();
    CollisionAudio(const CollisionAudio&)=delete;
    CollisionAudio& operator=(const CollisionAudio&)=delete;
    bool initialize(std::string& error);
    bool isAvailable() const noexcept;
    void playImpact(double normalClosingSpeed,bool cushion=false) noexcept;
    void setMuted(bool muted) noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace pool
