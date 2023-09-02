#include "stream_reassembler.hh"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    _next_index = 0;
    _eof_index = -1;
}

void StreamReassembler::write_buffer(size_t buffer_pos, const std::string &data, const size_t pos, const size_t len) {
    size_t new_size = buffer_pos + len;
    if (new_size > _buffer.size()) {
        if (new_size + _output.buffer_size() <= _capacity) {
            // grow _buffer
            _buffer.resize(new_size, '#');
            _flag.resize(new_size, false);
        } else {
            // silent discard
            _eof_index = -1;
            new_size = _capacity - _output.buffer_size();
            _buffer.resize(new_size, '#');
            _flag.resize(new_size, false);
        }
    }

    for (size_t i = pos; i < pos + len; i++, buffer_pos++) {
        if (_flag[buffer_pos] == false) {
            _buffer[buffer_pos] = data[i];
            _flag[buffer_pos] = true;
            _unassembled_bytes++;
        }
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _eof_index = index + data.size();
    }

    if (index > _next_index) {
        const size_t buffer_pos = index - _next_index;
        size_t len = min(_capacity - _output.buffer_size(), data.size());
        write_buffer(buffer_pos, data, 0, len);
    } else {
        const size_t data_pos = _next_index - index + 0;
        size_t data_len = data.size() > data_pos ? data.size() - data_pos : 0;
        size_t len = min(_capacity - _output.buffer_size(), data_len);
        write_buffer(0, data, data_pos, len);

        string next_segment;
        while (!_flag.empty() && _flag.front()) {
            next_segment.push_back(_buffer.front());
            _buffer.pop_front();
            _flag.pop_front();
            _next_index++;
            _unassembled_bytes--;
        }

        if (_next_index == _eof_index) {
            _output.end_input();
        }

        _output.write(next_segment);
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _output.buffer_empty() && _buffer.empty(); }
