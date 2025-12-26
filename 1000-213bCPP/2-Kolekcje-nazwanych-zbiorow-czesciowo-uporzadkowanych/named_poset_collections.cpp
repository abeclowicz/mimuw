#include <array>
#include <bitset>
#include <cstddef>
#include <map>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>

#include "named_poset_collections.h"

#ifndef N
    #define N 32
#endif

using std::advance;
using std::array;
using std::bitset;
using std::distance;
using std::map;
using std::max;
using std::nullopt;
using std::optional;
using std::regex;
using std::regex_match;
using std::string;
using std::unordered_map;

namespace {
    constexpr size_t SIZE = N;

    using relation_matrix = array<bitset<SIZE>, SIZE>;
    using poset = map<string, relation_matrix>;
    using npc = unordered_map<long, poset>;

    npc &get_collections() {
        static npc collections;
        return collections;
    }

    // Returns relation matrix with entries (x, x) for 0 ≤ x < N.
    const relation_matrix &get_diagonal_matrix() {
        static constexpr relation_matrix diagonal_matrix = [] {
            relation_matrix matrix;

            for (size_t i = 0; i < SIZE; i++) {
                matrix[i].set(i);
            }

            return matrix;
        }();

        return diagonal_matrix;
    }

    bool is_valid_name(const string &name) {
        static const regex re("^[a-zA-Z0-9_]+$");
        return regex_match(name, re);
    }

    optional<npc::iterator> find_collection(const long id) {
        npc &collections = get_collections();
        auto it = collections.find(id);

        if (it == collections.end()) {
            return nullopt;
        }

        return it;
    }

    optional<poset::iterator> find_poset(const long id, char const *name,
                                         const bool next = false) {
        const auto opt_it = find_collection(id);

        if (opt_it.has_value()) {
            auto it = (*opt_it)->second.find(name);

            if (next && it != (*opt_it)->second.end()) {
                ++it;
            }

            if (it != (*opt_it)->second.end()) {
                return it;
            }
        }

        return nullopt;
    }
} /* namespace */

namespace cxx {
    long npc_new_collection(void) {
        static long new_id = 0;

        if (new_id >= 0) {
            get_collections()[new_id] = {};
            return new_id++;
        }

        return -1;
    }

    void npc_delete_collection(long id) {
        const auto opt_it = find_collection(id);

        if (opt_it.has_value()) {
            get_collections().erase(*opt_it);
        }
    }

    bool npc_new_poset(long id, char const *name) {
        if (!name || !is_valid_name(name)) {
            return false;
        }

        const auto opt_it = find_collection(id);

        if (opt_it.has_value()) {
            auto &posets = (*opt_it)->second;

            if (!posets.contains(name)) {
                posets.emplace(string(name), get_diagonal_matrix());
                return true;
            }
        }

        return false;
    }

    void npc_delete_poset(long id, char const *name) {
        if (name) {
            const auto opt_it = find_collection(id);

            if (opt_it.has_value()) {
                (*opt_it)->second.erase(name);
            }
        }
    }

    bool npc_copy_poset(long id, char const *name_dst, char const *name_src) {
        if (!name_dst || !name_src || !is_valid_name(name_dst)) {
            return false;
        }

        const auto opt_it = find_poset(id, name_src);

        if (opt_it.has_value()) {
            get_collections()[id][name_dst] = (*opt_it)->second;
            return true;
        }

        return false;
    }

    char const *npc_first_poset(long id) {
        const auto opt_it = find_collection(id);

        if (!opt_it.has_value() || (*opt_it)->second.empty()) {
            return nullptr;
        }

        return (*opt_it)->second.begin()->first.c_str();
    }

    char const *npc_next_poset(long id, char const *name) {
        if (!name) {
            return nullptr;
        }

        const auto opt_it = find_poset(id, name, true);

        if (!opt_it.has_value()) {
            return nullptr;
        }

        return (*opt_it)->first.c_str();
    }

    bool npc_add_relation(long id, char const *name, size_t x, size_t y) {
        if (!name || max(x, y) >= SIZE) {
            return false;
        }

        const auto opt_it = find_poset(id, name);

        if (!opt_it.has_value()) {
            return false;
        }

        auto &matrix = (*opt_it)->second;

        if (matrix[x].test(y) || matrix[y].test(x)) {
            return false;
        }

        // Performs a transitive closure.
        for (size_t z = 0; z < SIZE; z++) {
            if (matrix[z].test(x)) {
                matrix[z] |= matrix[y];
            }
        }

        return true;
    }

    bool npc_is_relation(long id, char const *name, size_t x, size_t y) {
        if (!name || max(x, y) >= SIZE) {
            return false;
        }

        const auto opt_it = find_poset(id, name);

        return opt_it.has_value() && (*opt_it)->second[x].test(y);
    }

    bool npc_remove_relation(long id, char const *name, size_t x, size_t y) {
        if (!name || x == y || max(x, y) >= SIZE) {
            return false;
        }

        const auto opt_it = find_poset(id, name);

        if (!opt_it.has_value()) {
            return false;
        }

        auto &matrix = (*opt_it)->second;

        if (!matrix[x].test(y)) {
            return false;
        }

        // Verifies that x and y are not indirectly related.
        for (size_t z = 0; z < SIZE; z++) {
            if (z != x && z != y && matrix[x].test(z) && matrix[z].test(y)) {
                return false;
            }
        }

        matrix[x].reset(y);

        return true;
    }

    size_t npc_size() {
        return get_collections().size();
    }

    size_t npc_poset_size() {
        return SIZE;
    }

    size_t npc_collection_size(long id) {
        const auto opt_it = find_collection(id);

        if (opt_it.has_value()) {
            return (*opt_it)->second.size();
        }

        return 0;
    }
} /* namespace cxx */
