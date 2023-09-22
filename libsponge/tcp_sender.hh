#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <map>
#include <queue>
#include <set>
#include <utility>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    class SegCmp {
      public:
        bool operator()(const std::pair<uint64_t, TCPSegment> &lhs, const std::pair<uint64_t, TCPSegment> &rhs) const {
            return lhs.first < rhs.first;
        }
    };
    class ReTransmitTimer {
      public:
        explicit ReTransmitTimer(uint64_t rto) : _time(rto) {}

        bool expired() const { return _time == 0 && !_stopped; }

        void elapse(uint64_t ms) {
            if (_stopped)
                return;
            if (ms >= _time) {
                _time = 0;
            } else {
                _time -= ms;
            }
        }

        void reset(uint64_t rto) {
            _time = rto;
            _stopped = false;
        }

        void stop() {
            _stopped = true;
            _time = 0;
        }

        bool stopped() const { return _stopped; }

      private:
        uint64_t _time{0};
        bool _stopped{false};
    };

  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    std::set<std::pair<uint64_t, TCPSegment>, SegCmp> _segments_in_flight{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;
    ReTransmitTimer _timer;

    //! the (absolute) sequence number for the next byte to be sent
    uint64_t _next_seqno{0};

    uint64_t _window_size{1};

    uint64_t _consecutive_retransmissions{0};

    uint64_t _rto{0};

    uint64_t _ackno{0};  // checkpoint

    uint64_t _bytes_in_flight{0};

    size_t _get_window_size() const;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    void ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
