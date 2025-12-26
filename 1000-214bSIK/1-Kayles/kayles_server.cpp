#include <algorithm>
#include <bits/chrono.h>
#include <cstdint>
#include <ctime>
#include <exception>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "argparse.h"
#include "messages.h"
#include "sockets.h"

using std::all_of;
using std::cerr;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::steady_clock;
using std::chrono::time_point;
using std::exception;
using std::make_pair;
using std::max;
using std::min;
using std::nullopt;
using std::optional;
using std::runtime_error;
using std::set;
using std::system_error;
using std::unordered_map;
using std::vector;

using timestamp_t = time_point<steady_clock>;

#define TIME_NOW steady_clock::now()

struct GameIDWithTimestamp {
    uint32_t game_id;
    timestamp_t timestamp;

    GameIDWithTimestamp(uint32_t game_id, const timestamp_t& timestamp)
        : game_id{game_id}
        , timestamp{timestamp} {}

    bool operator<(const GameIDWithTimestamp& other) const {
        return make_pair(timestamp, game_id) < make_pair(other.timestamp, other.game_id);
    }
};

set<GameIDWithTimestamp> active_game_ids_with_timestamps;
set<GameIDWithTimestamp> finished_game_ids_with_timestamps;

class Game {
    public:
        State state;
        timestamp_t player_a_last_valid_msg;
        timestamp_t player_b_last_valid_msg;

        Game(const State& state, const timestamp_t& now = TIME_NOW)
            : state(state)
            , player_a_last_valid_msg(now)
            , player_b_last_valid_msg(now) {}

        void insert_to_set() const {
            is_active()
                ? active_game_ids_with_timestamps.insert(get_active_game_id_with_timestamp())
                : finished_game_ids_with_timestamps.insert(get_finished_game_id_with_timestamp());
        }

        void erase_from_set() const {
            is_active()
                ? active_game_ids_with_timestamps.erase(get_active_game_id_with_timestamp())
                : finished_game_ids_with_timestamps.erase(get_finished_game_id_with_timestamp());
        }

    private:
        bool is_active() const {
            return state.status == WAITING_FOR_OPPONENT || state.status == TURN_A || state.status == TURN_B;
        }

        GameIDWithTimestamp get_active_game_id_with_timestamp() const {
            if (state.status == WAITING_FOR_OPPONENT) {
                return GameIDWithTimestamp(state.game_id, player_a_last_valid_msg);
            }

            return GameIDWithTimestamp(state.game_id, min(player_a_last_valid_msg, player_b_last_valid_msg));
        }

        GameIDWithTimestamp get_finished_game_id_with_timestamp() const {
            return GameIDWithTimestamp(state.game_id, max(player_a_last_valid_msg, player_b_last_valid_msg));
        }
};

unordered_map<uint32_t, Game> games;

int64_t seconds_since(const timestamp_t& start, const timestamp_t& end = TIME_NOW) {
    return duration_cast<seconds>(end - start).count();
}

void update_timed_out_games(time_t timeout, const timestamp_t& now = TIME_NOW) {
    auto it_active = active_game_ids_with_timestamps.begin();

    while (it_active != active_game_ids_with_timestamps.end()) {
        if (seconds_since(it_active->timestamp, now) >= timeout) {
            Game& game = games.at(it_active->game_id);

            if (game.state.status == WAITING_FOR_OPPONENT) {
                games.erase(game.state.game_id);
            }
            else {
                timestamp_t lhs = game.player_a_last_valid_msg;
                timestamp_t rhs = game.player_b_last_valid_msg;

                game.state.status = (lhs < rhs || (lhs == rhs && game.state.status == TURN_A)) ? WIN_B : WIN_A;
                game.insert_to_set();
            }

            it_active = active_game_ids_with_timestamps.erase(it_active);
        }
        else {
            break;
        }
    }

    auto it_finished = finished_game_ids_with_timestamps.begin();

    while (it_finished != finished_game_ids_with_timestamps.end()) {
        if (seconds_since(it_finished->timestamp, now) >= timeout) {
            games.erase(it_finished->game_id);
            it_finished = finished_game_ids_with_timestamps.erase(it_finished);
        }
        else {
            break;
        }
    }
}

State handle_message(const Message& message, const vector<bool>& pawn_row,
                     const timestamp_t& now = TIME_NOW)
{
    if (!message.is_valid()) {
        return message.to_wrong_message_state();
    }

    static uint32_t next_game_id = 0;
    static optional<uint32_t> waiting_game_id = nullopt;

    if (message.msg_type == JOIN) {
        if (waiting_game_id.has_value() && games.contains(waiting_game_id.value())) {
            Game& game = games.at(waiting_game_id.value());

            game.erase_from_set();

            game.state.player_b_id = message.player_id;
            game.state.status = TURN_B;
            game.player_b_last_valid_msg = now;

            game.insert_to_set();

            waiting_game_id = nullopt;
            return game.state;
        }

        if (next_game_id == UINT32_MAX) {
            throw runtime_error("Game ID pool has been exhausted");
        }

        const State state(next_game_id++, message.player_id, 0, WAITING_FOR_OPPONENT, pawn_row.size() - 1,
                          pawn_row);
        const Game game(state, now);

        games.emplace(state.game_id, game);
        game.insert_to_set();

        waiting_game_id = state.game_id;
        return state;
    }

    auto it = games.find(message.game_id);

    if (it == games.end()) {
        return message.to_wrong_message_state(5); // 6th byte -> game_id
    }

    Game& game = it->second;
    State& state = game.state;

    if (message.player_id != state.player_a_id && message.player_id != state.player_b_id) {
        return message.to_wrong_message_state(1); // 2nd byte -> player_id
    }

    game.erase_from_set();

    bool is_player_a = message.player_id == state.player_a_id;
    bool is_player_b = message.player_id == state.player_b_id;

    if (is_player_a) {
        game.player_a_last_valid_msg = now;
    }

    if (is_player_b) {
        game.player_b_last_valid_msg = now;
    }

    bool can_move = (is_player_a && state.status == TURN_A) || (is_player_b && state.status == TURN_B);

    if (can_move) {
        bool changed_state = (message.msg_type == GIVE_UP);

        if (message.msg_type == MOVE_1 || message.msg_type == MOVE_2) {
            static const auto has_pawn = [](const State& state, uint8_t pawn) {
                return (pawn <= state.max_pawn) &&
                       static_cast<bool>(state.pawn_row[pawn / 8] & (0x80U >> (pawn % 8)));
            };

            static const auto take_pawn = [](State& state, uint8_t pawn) {
                if (pawn <= state.max_pawn) {
                    state.pawn_row[pawn / 8] ^= (0x80U >> (pawn % 8));
                }
            };

            if (message.msg_type == MOVE_1) {
                if (has_pawn(state, message.pawn)) {
                    take_pawn(state, message.pawn);
                    changed_state = true;
                }
            }
            else {
                if (has_pawn(state, message.pawn) && message.pawn < UINT8_MAX &&
                    has_pawn(state, message.pawn + 1))
                {
                    take_pawn(state, message.pawn);
                    take_pawn(state, message.pawn + 1);

                    changed_state = true;
                }
            }
        }

        if (changed_state) {
            if (message.msg_type == GIVE_UP) {
                state.status = (state.status == TURN_A) ? WIN_B : WIN_A;
            }
            else {
                bool clean = all_of(state.pawn_row.begin(), state.pawn_row.end(), [](char c) {
                    return c == 0;
                });

                if (clean) {
                    state.status = (state.status == TURN_A) ? WIN_A : WIN_B;
                }
                else {
                    state.status = (state.status == TURN_A) ? TURN_B : TURN_A;
                }
            }
        }
    }

    game.insert_to_set();

    return state;
}

int main(int argc, char *argv[]) {
    try {
        auto [pawn_row, address, port, timeout] =
            parse_args<Arg::PAWN_ROW, Arg::ADDRESS, Arg::PORT, Arg::TIMEOUT>(argc, argv);

        ServerSocket socket(address, port);

        while (true) {
            try {
                auto [message, addr] = socket.receive();

                update_timed_out_games(timeout);

                const State state = handle_message(message, pawn_row);

                socket.send(state, addr);
            } catch (const exception& ex) {
                #ifdef DEBUG
                cerr << "[debug] Error : " << ex.what() << '\n';
                #endif /* DEBUG */
            }
        }
    }
    catch (const exception& ex) {
        cerr << "Error : " << ex.what() << '\n';
        return 1;
    }
}
