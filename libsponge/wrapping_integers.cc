#include "wrapping_integers.hh"

#include <bits/stdint-intn.h>
#include <bits/stdint-uintn.h>
#include <cstdint>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    DUMMY_CODE(n, isn);
    return isn + (static_cast<uint32_t>(n & (UINT32_MAX)));
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    DUMMY_CODE(n, isn, checkpoint);
    uint64_t extra = (checkpoint & (~static_cast<uint64_t>(UINT32_MAX))) >> 32;
    uint32_t offset = n - isn;

    if (extra == 0) {
        uint64_t x = offset;
        uint64_t x1 = (static_cast<uint64_t>(1) << 32) | offset;
        if (x > checkpoint) {
            return x;
        } else {
            uint64_t d1 = checkpoint - x;
            uint64_t d2 = x1 - checkpoint;
            if (d1 < d2) {
                return x;
            } else {
                return x1;
            }
        }
    } else {
        uint64_t x0 = ((extra - 1ULL) << 32) | offset;
        uint64_t x = (extra << 32) | offset;
        uint64_t x2 = ((extra + 1ULL) << 32) | offset;
        if (checkpoint >= x) {
            uint64_t d1 = checkpoint - x;
            uint64_t d2 = x2 - checkpoint;
            if (d1 < d2) {
                return x;
            } else {
                return x2;
            }
        } else {
            uint64_t d1 = checkpoint - x0;
            uint64_t d2 = x - checkpoint;
            if (d1 < d2) {
                return x0;
            } else {
                return x;
            }
        }
    }

    return {};
}
