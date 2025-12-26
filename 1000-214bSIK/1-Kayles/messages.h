#ifndef MESSAGES_H
#define MESSAGES_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <vector>

#define WAITING_FOR_OPPONENT 0
#define TURN_A 1
#define TURN_B 2
#define WIN_A 3
#define WIN_B 4
#define WRONG_MSG 255

#define JOIN 0
#define MOVE_1 1
#define MOVE_2 2
#define KEEP_ALIVE 3
#define GIVE_UP 4

namespace detail {

    class Serializable {
        public:
            virtual ~Serializable();

            virtual std::vector<char> serialize() const = 0;

        private:
            virtual void validate() const = 0;
    };

    struct [[gnu::packed]] PackedState {
        uint32_t game_id;
        uint32_t player_a_id;
        uint32_t player_b_id;
        uint8_t status;
        uint8_t max_pawn;
    };

    struct [[gnu::packed]] PackedWrongMessageState {
        char bytes[12];
        uint8_t status;
        uint8_t error_index;
    };

    struct [[gnu::packed]] PackedMessage {
        uint8_t msg_type;
        uint32_t player_id;
        uint32_t game_id;
        uint8_t pawn;
    };

    int message_field_count(uint8_t msg_type);

    std::size_t message_size(uint8_t msg_type);

} /* namespace detail */

class State : public detail::Serializable {
    public:
        uint32_t game_id;
        uint32_t player_a_id;
        uint32_t player_b_id;
        uint8_t status;
        uint8_t max_pawn;
        std::vector<char> pawn_row;

        State(uint32_t game_id, uint32_t player_a_id, uint32_t player_b_id, uint8_t status, uint8_t max_pawn,
              const std::vector<bool>& pawn_row);

        std::vector<char> serialize() const override;

        static std::size_t MAX_SIZE;

        static State deserialize(const std::vector<char>& bytes);

        friend std::ostream& operator<<(std::ostream& os, const State& state);

    private:
        State(const detail::PackedState& packed_state, const std::vector<bool>& pawn_row);

        detail::PackedState packed(bool network_order = false) const;

        void validate() const override;
};

class Message : public detail::Serializable {
    public:
        uint8_t msg_type;
        uint32_t player_id;
        uint32_t game_id;
        uint8_t pawn;

        Message(uint8_t msg_type, uint32_t player_id, uint32_t game_id, uint8_t pawn, bool throws = true);

        std::vector<char> serialize() const override;

        bool is_valid() const;

        State to_wrong_message_state(uint8_t error_index) const;

        State to_wrong_message_state() const;

        static std::size_t MAX_SIZE;

        static Message deserialize(const std::vector<char>& bytes, bool throws = true);

    private:
        mutable struct {
            char bytes[sizeof(detail::PackedWrongMessageState::bytes)];
            std::optional<uint8_t> error_index;
        } metadata;

        Message(const detail::PackedMessage& packed_message, bool throws = true);

        detail::PackedMessage packed(bool network_order = false) const;

        void validate() const override;
};

#endif /* MESSAGES_H */
