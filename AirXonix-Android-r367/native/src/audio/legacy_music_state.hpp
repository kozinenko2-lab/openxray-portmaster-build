#pragma once
#include <algorithm>
#include <cstddef>

namespace airxonix {

struct LegacyMusicSwitchRequest {
    bool pending = false;
    std::size_t trackId = 0;
    float fadeInPerMs = 0.0f;
};

// Direct transcription of the control state manipulated by 0x40B240,
// 0x40B260 and 0x40B0D0. File mapping/SDL playback is intentionally left to
// MusicPlayer; this type owns only the original pending/fade/active contract.
class LegacyMusicTransitionState {
public:
    void reset(bool streamActive = false, float gain = 1.0f) {
        pendingTrack_ = -1;
        pendingFadeIn_ = 0.0f;
        currentGain_ = std::clamp(gain, 0.0f, 1.0f);
        fadeRatePerMs_ = 0.0f;
        streamActive_ = streamActive;
    }

    void markDirectStreamOpen(bool open) {
        streamActive_ = open;
        pendingTrack_ = -1;
        pendingFadeIn_ = 0.0f;
        fadeRatePerMs_ = 0.0f;
        currentGain_ = open ? 1.0f : 0.0f;
    }

    // 0x40B240: request does not alter current fade state.
    void requestTrack(std::size_t trackId, float fadeInPerMs) {
        pendingTrack_ = static_cast<int>(trackId);
        pendingFadeIn_ = fadeInPerMs;
    }

    // 0x40B260: stores the negative argument and cancels any older request.
    void beginFadeOut(float perMs) {
        fadeRatePerMs_ = -perMs;
        pendingTrack_ = -1;
    }

    // 0x40B0D0. Boundary tests are deliberately strict: the original only
    // clamps after crossing >1 or <0, not when landing exactly on 1/0.
    LegacyMusicSwitchRequest update(int dtMs, bool streamIdleHandshake = true) {
        if (streamActive_) {
            if (fadeRatePerMs_ > 0.0f) {
                currentGain_ += static_cast<float>(dtMs) * fadeRatePerMs_;
                if (currentGain_ > 1.0f) {
                    currentGain_ = 1.0f;
                    fadeRatePerMs_ = 0.0f;
                }
            } else if (fadeRatePerMs_ < 0.0f) {
                currentGain_ += static_cast<float>(dtMs) * fadeRatePerMs_;
                if (currentGain_ < 0.0f) {
                    currentGain_ = 0.0f;
                    fadeRatePerMs_ = 0.0f;
                    streamActive_ = false;
                }
            }
            return {};
        }

        if (pendingTrack_ >= 0 && streamIdleHandshake) {
            LegacyMusicSwitchRequest r;
            r.pending = true;
            r.trackId = static_cast<std::size_t>(pendingTrack_);
            r.fadeInPerMs = pendingFadeIn_;
            return r;
        }
        return {};
    }

    // Called after the native file-open attempt. 0x40B0D0 clears pendingTrack
    // regardless of success; on success it marks stream active and copies the
    // caller transition scalar into fadeRate.
    void completeSwitch(bool opened) {
        if (opened) {
            streamActive_ = true;
            fadeRatePerMs_ = pendingFadeIn_;
        }
        pendingTrack_ = -1;
    }

    float currentGain() const { return currentGain_; }
    float fadeRatePerMs() const { return fadeRatePerMs_; }
    bool streamActive() const { return streamActive_; }
    int pendingTrack() const { return pendingTrack_; }
    float pendingFadeInPerMs() const { return pendingFadeIn_; }

private:
    int pendingTrack_ = -1;
    float pendingFadeIn_ = 0.0f;
    float currentGain_ = 1.0f;
    float fadeRatePerMs_ = 0.0f;
    bool streamActive_ = false;
};

} // namespace airxonix
