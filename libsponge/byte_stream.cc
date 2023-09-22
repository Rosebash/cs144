#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
    _cap = capacity;
    _input_ended = false;
}

size_t ByteStream::write(const string &data) {
    size_t remain = remaining_capacity();
    size_t len = min(data.size(), remain);
    _w_bytes_count += len;
    _stream.insert(_stream.end(), data.begin(), data.begin() + len);
    return len;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string res(_stream.begin(), _stream.begin() + min(len, buffer_size()));
    return res;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t len2 = min(len, buffer_size());
    _stream.erase(_stream.begin(), _stream.begin() + len2);
    _r_bytes_count += len2;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string front = peek_output(len);
    pop_output(len);
    return front;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _stream.size(); }

bool ByteStream::buffer_empty() const { return _stream.empty(); }

bool ByteStream::eof() const { return _input_ended && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _w_bytes_count; }

size_t ByteStream::bytes_read() const { return _r_bytes_count; }

size_t ByteStream::remaining_capacity() const { return _cap - buffer_size(); }
