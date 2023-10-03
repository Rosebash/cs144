#include "tcp_sender.hh"

#include "iostream"
#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _timer(retx_timeout)
    , _rto(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

size_t TCPSender::_get_window_size() const {
    uint64_t window_size = _window_size;
    window_size = std::max(static_cast<uint64_t>(1), window_size);
    if (_ackno + window_size >= _next_seqno) {
        window_size = _ackno + window_size - _next_seqno;
    } else {
        window_size = 0;
    }
    return window_size;
}

void TCPSender::fill_window() {
    size_t window_size = _get_window_size();  // sender window size
    while (window_size > 0 && (!_stream.buffer_empty() || CLOSED() || SYN_ACKED_FIN_TO_BE_SENT())) {
        TCPSegment seg;
        seg.header().seqno = next_seqno();
        seg.header().syn = next_seqno_absolute() == 0;

        seg.payload() = _stream.read(min(window_size - seg.header().syn, TCPConfig::MAX_PAYLOAD_SIZE));

        if (_stream.eof()) {
            if (seg.length_in_sequence_space() + 1 <= window_size) {
                seg.header().fin = true;
            }
        }

        size_t seq_len = seg.length_in_sequence_space();
        _segments_out.push(seg);
        _segments_in_flight.insert({next_seqno_absolute() + seq_len, seg});
        _next_seqno += seq_len;
        _bytes_in_flight += seq_len;
        window_size -= seq_len;

        if (_timer.stopped()) {
            _timer.reset(_rto);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _window_size = window_size;
    bool segment_received = false;
    for (auto it = _segments_in_flight.begin(); it != _segments_in_flight.end();) {
        const auto &last_seq = it->first;
        uint64_t ackno_absolute = unwrap(ackno, _isn, _ackno);
        if (ackno_absolute >= last_seq
            && ackno_absolute <= next_seqno_absolute()) {
            _bytes_in_flight -= it->second.length_in_sequence_space();
            _ackno = last_seq;
            it = _segments_in_flight.erase(it);
            segment_received = true;
        } else {
            ++it;
            break;
        }
    }
    if (segment_received) {
        _rto = _initial_retransmission_timeout;
        if (!_segments_in_flight.empty()) {
            _timer.reset(_rto);
        }
        _consecutive_retransmissions = 0;
    }

    if (_segments_in_flight.empty()) {
        _timer.stop();
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t mmm) {
    _timer.elapse(mmm);
    if (_timer.expired()) {
        if (!_segments_in_flight.empty()) {
            // TODO: retransmit the earlist segment that hasn't been fully acked
            if (_window_size > 0) {
                _consecutive_retransmissions++;
                _rto *= 2;
            }
            if (_consecutive_retransmissions <= TCPConfig::MAX_RETX_ATTEMPTS)
                _segments_out.push(_segments_in_flight.begin()->second);
            _timer.reset(_rto);
        }
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
