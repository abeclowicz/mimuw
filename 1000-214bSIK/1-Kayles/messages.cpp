#include <algorithm>
#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "messages.h"

namespace detail {

    std::vector<char> bits_to_bytes(const std::vector<bool>& bits) {
        std::vector<char> bytes((bits.size() + 7) / 8, 0);

        for (std::size_t i = 0; i < bits.size(); ++i) {
            if (bits[i]) {
                bytes[i / 8] |= (0x80U >> (i % 8));
            }
        }

        return bytes;
    }

    std::vector<bool> byte_to_bits(char byte) {
        std::vector<bool> bits;

        for (uint8_t i = 0; i < 8; ++i) {
            bits.push_back(static_cast<bool>(byte & (0x80U >> i)));
        }

        return bits;
    }

    int message_field_count(uint8_t msg_type) {
        static const std::unordered_map<uint8_t, int> msg_type_to_field_count {
            {JOIN, 2}, {MOVE_1, 4}, {MOVE_2, 4}, {KEEP_ALIVE, 3}, {GIVE_UP, 3}
        };

        auto it = msg_type_to_field_count.find(msg_type);
        return (it != msg_type_to_field_count.end()) ? it->second : 1;
    }

    std::size_t message_size(uint8_t msg_type) {
        static const std::unordered_map<uint8_t, std::size_t> msg_type_to_size {
            {JOIN, sizeof(detail::PackedMessage::msg_type) + sizeof(detail::PackedMessage::player_id)},
            {MOVE_1, sizeof(detail::PackedMessage)},
            {MOVE_2, sizeof(detail::PackedMessage)},
            {KEEP_ALIVE, sizeof(detail::PackedMessage) - sizeof(detail::PackedMessage::pawn)},
            {GIVE_UP, sizeof(detail::PackedMessage) - sizeof(detail::PackedMessage::pawn)}
        };

        auto it = msg_type_to_size.find(msg_type);
        return (it != msg_type_to_size.end()) ? it->second : sizeof(msg_type);
    }

    PackedState deserialize_packed_state(const std::vector<char>& bytes) {
        if (bytes.size() < sizeof(PackedState)) {
            throw std::invalid_argument("Not enough bytes to deserialize state");
        }

        PackedState packed_state;
        std::memcpy(&packed_state, bytes.data(), sizeof(packed_state));

        // Note: game_id, player_a_id and player_b_id may form an array of bytes.
        if (packed_state.status != WRONG_MSG) {
            packed_state.game_id = ntohl(packed_state.game_id);
            packed_state.player_a_id = ntohl(packed_state.player_a_id);
            packed_state.player_b_id = ntohl(packed_state.player_b_id);
        }

        return packed_state;
    }

    PackedMessage deserialize_packed_message(const std::vector<char>& bytes) {
        if (bytes.size() >= sizeof(detail::PackedMessage::msg_type)) {
            // Note: bytes[0] is the message type.
            if (std::size_t size = message_size(bytes[0]); bytes.size() >= size) {
                PackedMessage packed_message{};
                std::memcpy(&packed_message, bytes.data(), size);

                packed_message.player_id = ntohl(packed_message.player_id);
                packed_message.game_id = ntohl(packed_message.game_id);

                return packed_message;
            }
        }

        throw std::invalid_argument("Not enough bytes to deserialize message");
    }

    Serializable::~Serializable() = default;

} /* namespace detail */

State::State(uint32_t game_id, uint32_t player_a_id, uint32_t player_b_id, uint8_t status, uint8_t max_pawn,
             const std::vector<bool>& pawn_row)
    : game_id{game_id}
    , player_a_id{player_a_id}
    , player_b_id{player_b_id}
    , status{status}
    , max_pawn{max_pawn}
    , pawn_row{detail::bits_to_bytes(pawn_row)}
{
    if (status != WRONG_MSG) {
        if (pawn_row.empty()) {
            throw std::invalid_argument("Pawn row cannot be empty");
        }

        if (pawn_row.size() - 1 > UINT8_MAX) {
            throw std::invalid_argument("Pawn row can be at most 256 characters long");
        }

        if (max_pawn != pawn_row.size() - 1) {
            throw std::invalid_argument("Maximum pawn index does not match the pawn row's length");
        }
    }

    validate();
}

std::vector<char> State::serialize() const {
    const detail::PackedState packed_state = packed(true);

    auto ptr = reinterpret_cast<const char*>(&packed_state);
    std::vector<char> bytes(ptr, ptr + sizeof(packed_state));

    if (status != WRONG_MSG) {
        bytes.insert(bytes.end(), pawn_row.begin(), pawn_row.end());
    }

    return bytes;
}

std::size_t State::MAX_SIZE = sizeof(detail::PackedState) + (UINT8_MAX / 8) + 1;

State State::deserialize(const std::vector<char>& bytes) {
    const detail::PackedState packed_state = detail::deserialize_packed_state(bytes);

    std::vector<bool> pawn_row;

    for (std::size_t i = sizeof(packed_state); i < bytes.size(); ++i) {
        const std::vector<bool> bits = detail::byte_to_bits(bytes[i]);
        pawn_row.insert(pawn_row.end(), bits.begin(), bits.end());
    }

    // Note: try to 'fit' pawn_row to the expected length.
    for (uint8_t i = 0; i < 7 - (packed_state.max_pawn % 8); ++i) {
        if (!pawn_row.empty() && !pawn_row.back()) {
            pawn_row.pop_back();
        }
        else {
            break;
        }
    }

    return State(packed_state, pawn_row);
}

std::ostream& operator<<(std::ostream& os, const State& state) {
    if (state.status != WRONG_MSG) {
        static const std::unordered_map<uint8_t, std::string_view> status_to_str {
            {WAITING_FOR_OPPONENT, "WAITING_FOR_OPPONENT"},
            {TURN_A, "TURN_A"},
            {TURN_B, "TURN_B"},
            {WIN_A, "WIN_A"},
            {WIN_B, "WIN_B"}
        };

        auto it = status_to_str.find(state.status);
        std::string_view status_str = (it != status_to_str.end()) ? it->second : "unknown";

        os << "Game ID     : " << state.game_id << '\n';
        os << "Player A ID : " << state.player_a_id << '\n';
        os << "Player B ID : " << state.player_b_id << '\n';
        os << "Status      : " << status_str << '\n';
        os << "Pawns       : ";

        for (uint8_t i = 0; i <= state.max_pawn; ++i) {
            os << static_cast<bool>(state.pawn_row[i / 8] & (0x80U >> (i % 8)));
        }

        return os;
    }

    const detail::PackedState packed_state = state.packed();
    auto ptr = reinterpret_cast<const detail::PackedWrongMessageState*>(&packed_state);

    os << "Wrong message : invalid byte at position " << static_cast<uint32_t>(ptr->error_index) << '\n';

    for (uint8_t i = 0; i < sizeof(ptr->bytes); ++i) {
        os << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(ptr->bytes[i]);
        os << (i < sizeof(ptr->bytes) - 1 ? ' ' : '\n');
    }

    os << std::resetiosflags(std::ios_base::basefield); // undo std::hex

    if (ptr->error_index < sizeof(ptr->bytes)) {
        for (uint8_t i = 0; i < ptr->error_index; ++i) {
            os << "~~~";
        }

        os << "^~";
    }

    return os;
}

State::State(const detail::PackedState& packed_state, const std::vector<bool>& pawn_row)
    : State(packed_state.game_id, packed_state.player_a_id, packed_state.player_b_id, packed_state.status,
            packed_state.max_pawn, pawn_row) {}

detail::PackedState State::packed(bool network_order) const {
    detail::PackedState packed_state {
        .game_id = game_id,
        .player_a_id = player_a_id,
        .player_b_id = player_b_id,
        .status = status,
        .max_pawn = max_pawn
    };

    // Note: game_id, player_a_id and player_b_id may form an array of bytes.
    if (network_order && packed_state.status != WRONG_MSG) {
        packed_state.game_id = htonl(packed_state.game_id);
        packed_state.player_a_id = htonl(packed_state.player_a_id);
        packed_state.player_b_id = htonl(packed_state.player_b_id);
    }

    return packed_state;
}

void State::validate() const {
    if (status != WRONG_MSG) {
        if (player_a_id == 0) {
            throw std::invalid_argument("Player A ID must be positive");
        }

        if (status > WIN_B) {
            throw std::invalid_argument("Unknown status");
        }

        if (status == WAITING_FOR_OPPONENT) {
            if (player_b_id > 0) {
                throw std::invalid_argument("Player B ID must be zero");
            }
        }
        else {
            if (player_b_id == 0) {
                throw std::invalid_argument("Player B ID must be positive");
            }
        }

        if (uint8_t remainder = max_pawn % 8; remainder > 0) {
            if (uint8_t mask = (0x80U >> remainder) - 1; pawn_row.back() & mask) {
                throw std::invalid_argument("Some unused bits in the pawn row are not zero");
            }
        }
    }
}

Message::Message(uint8_t msg_type, uint32_t player_id, uint32_t game_id, uint8_t pawn, bool throws)
    : msg_type{msg_type}
    , player_id{player_id}
    , game_id{game_id}
    , pawn{pawn}
    , metadata{}
{
    std::fill_n(metadata.bytes, sizeof(metadata.bytes), 0);
    metadata.error_index = std::nullopt;

    try {
        validate();
    }
    catch (...) {
        if (throws) {
            throw;
        }
    }
}

std::vector<char> Message::serialize() const {
    const detail::PackedMessage packed_message = packed(true);

    auto ptr = reinterpret_cast<const char*>(&packed_message);
    return std::vector<char>(ptr, ptr + detail::message_size(packed_message.msg_type));
}

bool Message::is_valid() const {
    return !metadata.error_index.has_value();
}

State Message::to_wrong_message_state(uint8_t error_index) const {
    detail::PackedState packed_state;

    auto ptr = reinterpret_cast<detail::PackedWrongMessageState*>(&packed_state);

    std::memcpy(ptr->bytes, metadata.bytes, sizeof(metadata.bytes));
    ptr->status = WRONG_MSG;
    ptr->error_index = error_index;

    return State(packed_state.game_id, packed_state.player_a_id, packed_state.player_b_id,
                 packed_state.status, packed_state.max_pawn, std::vector<bool>());
}

State Message::to_wrong_message_state() const {
    if (is_valid()) {
        throw std::invalid_argument("Can only convert invalid messages");
    }

    return to_wrong_message_state(metadata.error_index.value());
}

std::size_t Message::MAX_SIZE = std::max(sizeof(detail::PackedMessage), sizeof(Message::metadata.bytes));

Message Message::deserialize(const std::vector<char>& bytes, bool throws) {
    detail::PackedMessage packed_message{};
    bool not_enough_bytes = false;

    try {
        packed_message = detail::deserialize_packed_message(bytes);
    }
    catch (...) {
        if (throws) {
            throw;
        }

        not_enough_bytes = true;
    }

    Message message(packed_message, throws);

    std::size_t count = std::min(sizeof(message.metadata.bytes), bytes.size());
    std::copy_n(bytes.data(), count, message.metadata.bytes);

    if (not_enough_bytes) {
        message.metadata.error_index = bytes.size();
    }
    else if (message.is_valid() && bytes.size() > detail::message_size(message.msg_type)) {
        message.metadata.error_index = detail::message_size(message.msg_type);
    }

    return message;
}

Message::Message(const detail::PackedMessage& packed_message, bool throws)
    : Message(packed_message.msg_type, packed_message.player_id, packed_message.game_id, packed_message.pawn,
              throws) {}

detail::PackedMessage Message::packed(bool network_order) const {
    return detail::PackedMessage {
        .msg_type = msg_type,
        .player_id = network_order ? htonl(player_id) : player_id,
        .game_id = network_order ? htonl(game_id) : game_id,
        .pawn = pawn
    };
}

void Message::validate() const {
    if (msg_type > GIVE_UP) {
        metadata.error_index = 0;
        throw std::invalid_argument("Unknown message type");
    }

    if (msg_type == JOIN && player_id == 0) {
        metadata.error_index = 1;
        throw std::invalid_argument("Player ID must be positive");
    }
}
