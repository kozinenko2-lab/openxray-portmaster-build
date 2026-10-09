#include "legacy_random.hpp"

int LegacyRandom::next() {
    state_ = state_ * 0x343fdu + 0x269ec3u;
    return static_cast<int>((state_ >> 16u) & 0x7fffu);
}
