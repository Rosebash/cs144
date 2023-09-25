#include "tcp_receiver.hh"

#include <cstdio>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    DUMMY_CODE(seg);
    if (seg.header().syn) {
        _isn.emplace(seg.header().seqno);
    }
    if (!_isn.has_value()) {
        return;
    }

    WrappingInt32 n(seg.header().seqno);
    uint64_t checkpoint = stream_out().bytes_written() - 1 + 1;
    uint64_t index = unwrap(n, _isn.value(), checkpoint) - !seg.header().syn;
    _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (!_isn.has_value()) {
        return {};
    }

    return wrap(abs_ackno(), _isn.value());
}

size_t TCPReceiver::abs_ackno() const { return stream_out().bytes_written() + 1 + stream_out().input_ended(); }

size_t TCPReceiver::window_size() const { return _capacity - stream_out().bytes_written() + stream_out().bytes_read(); }
