#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPConnection::set_header(TCPSegment &seg) {
    if (_unclean_shutdown) {
        seg.header().rst = true;
    } else {
        std::optional<WrappingInt32> ackno = _receiver.ackno();
        size_t window_size = _receiver.window_size();
        if (ackno.has_value()) {
            seg.header().ack = true;
            seg.header().ackno = ackno.value();
            if (window_size > numeric_limits<uint16_t>::max())
                seg.header().win = numeric_limits<uint16_t>::max();
            else
                seg.header().win = window_size;
        }
    }
}

void TCPConnection::forward_segments() {
    auto &s_out = _sender.segments_out();

    while (!s_out.empty()) {
        TCPSegment seg = std::move(s_out.front());
        s_out.pop();

        set_header(seg);
        _segments_out.emplace(std::move(seg));  // real send
    }
}

void TCPConnection::unclean_shutdown() {
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _unclean_shutdown = true;
}

bool TCPConnection::active() const { return !(_unclean_shutdown || _clean_shutdown); }

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _now_time - _time_when_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_when_last_segment_received = _now_time;

    if (seg.header().rst) {
        unclean_shutdown();
    } else {
        _receiver.segment_received(seg);
        if (seg.header().ack) {
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }

        if (seg.length_in_sequence_space() > 0) {
            if (_segments_out.empty()) {
                _sender.send_empty_segment();
            }
        }
    }
}

size_t TCPConnection::write(const string &data) {
    size_t w_bytes_cnt = _sender.stream_in().write(data);
    _sender.fill_window();
    forward_segments();

    return w_bytes_cnt;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _now_time += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    forward_segments();

    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        unclean_shutdown();
        _sender.send_empty_segment();
        forward_segments();
    }

    if (_receiver.FIN_RECV() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    if (TIME_WAIT()) {
        bool close = (_linger_after_streams_finish == false) ||
                     (_now_time - _time_when_last_segment_received >= 10 * _cfg.rt_timeout);
        if (close) {
            _clean_shutdown = true;
        }
    }

    // TODO
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    forward_segments();
}

void TCPConnection::connect() {
    _sender.fill_window();
    forward_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            unclean_shutdown();
            _sender.send_empty_segment();
            forward_segments();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
