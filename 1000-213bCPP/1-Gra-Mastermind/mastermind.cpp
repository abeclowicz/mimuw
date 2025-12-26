#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
    constexpr std::string_view ERROR_MESSAGE = "ERROR\n";

    /**
     * Compares two sequences containing colors of individual pegs.
     * 
     * @param sequence sequence to compare with
     * @param target sequence to compare to ("secret" sequence)
     * 
     * @returns `{b, w}`, where `b` is the number of pegs correct in color and
     * position and `w` is the number of pegs from `sequence` correct in color
     * only
     */
    std::pair<int, int> compare_sequences(
        const std::vector<int> &sequence,
        const std::vector<int> &target
    ) {
        assert(sequence.size() == target.size());
    
        int b = 0, w = 0;
        std::unordered_map<int, int> unpaired_sequence, unpaired_target;
    
        for (std::size_t i = 0; i < sequence.size(); i++) {
            if (sequence[i] == target[i]) {
                b++;
            }
            else {
                unpaired_sequence[sequence[i]]++;
                unpaired_target[target[i]]++;
            }
        }

        // Matches unpaired pegs from sequence with unpaired pegs from target.
        for (const std::pair<const int, int> &elem : unpaired_sequence) {
            w += std::min(elem.second, unpaired_target[elem.first]);
        }
    
        return std::make_pair(b, w);
    }

    /**
     * Interprets an integer value in the `num` character array and stores it
     * into `value`.
     * 
     * @param value int to store the result into
     * @param num characters to convert
     * 
     * @returns `false` if `std::invalid_argument` or `std::out_of_range`
     * exceptions are thrown, `true` otherwise
     */
    bool parse_int(int &value, const char *num) {
        try {
            std::size_t idx = 0;
            value = std::stoi(num, &idx);
            return num[idx] == '\0';
        }
        catch (std::exception &) {
            return false;
        }
    }

    /**
     * Interprets `n` integer values in the `num` character array array and
     * stores it into `value`.
     * 
     * See `parse_int()` for more details.
     * 
     * @param value vector to store the result into
     * @param num array of characters to convert
     * @param n number of ints to parse
     * 
     * @return `false` if `std::invalid_argument` or `std::out_of_range`
     * exceptions are thrown, `true` otherwise
     */
    bool parse_n_ints(
        std::vector<int> &value,
        const char * const *num, const int n
    ) {
        value.resize(n);

        for (int i = 0; i < n; i++) {
            if (!parse_int(value[i], num[i])) {
                return false;
            }
        }

        return true;
    }

    /**
     * Reads `n` ints from the standard input into `input`.
     * 
     * @param input vector to read into
     * @param n number of ints to read
     * 
     * @returns `true` if input consists of exactly `n` single-space-separated
     * ints in a single line, `false` otherwise
     */
    bool read_n_ints(std::vector<int> &input, const int n) {
        // Single-space-separated nonnegative integers.
        static const std::regex regex_valid_line("[0-9]+( [0-9]+)*");

        std::string line;
        
        if (!std::getline(std::cin, line)) {
            // Checks if user closed the input stream.
            if (std::cin.eof()) {
                exit(0);
            }

            return false;
        }

        std::smatch data_fields;

        // Checks if the input is in the correct format.
        if (!std::regex_match(line, data_fields, regex_valid_line)) {
            return false;
        }

        static const std::regex regex_numeric("[0-9]+");

        auto it = std::sregex_iterator(
            line.begin(), line.end(), regex_numeric
        );

        int temp;
        
        input.clear();

        // Iterates over all integers from the input and tries to parse them.
        while (it != std::sregex_iterator()) {
            if (!parse_int(temp, (it++)->str().c_str())) {
                return false;
            }

            input.push_back(temp);
        }

        // Validates the number of read integers.
        if (static_cast<int>(input.size()) != n) {
            return false;
        }
        
        return true;  
    }
}

namespace Codebreaker {
    /**
     * Recursively generates all sequences of length `n`, with elements ranging
     * from 0 to `k - 1`.
     * 
     * @param sequences vector to store the generated sequences
     * @param k upper bound of sequence elements
     * @param n length of each sequence
     * @param sequence temporary vector that holds the current sequence
     */
    void get_every_sequence(
        std::vector<std::vector<int>> &sequences,
        const int k, const int n,
        std::vector<int> &sequence
    ) {
        // If the sequence is not yet of length n, appends every possible
        // element and recurses.
        if (static_cast<int>(sequence.size()) < n) {
            for (int i = 0; i < k; i++) {
                sequence.push_back(i);
                get_every_sequence(sequences, k, n, sequence);
                sequence.pop_back();
            }
    
            return;
        }
        
        sequences.push_back(sequence);
    }
    
    /**
     * Prints non-empty `sequence` to the standard output.
     */
    void print_sequence(const std::vector<int> &sequence) {
        std::cout << sequence[0];
        
        for (std::size_t i = 1; i < sequence.size(); i++) {
            std::cout << ' ' << sequence[i];
        }
    
        std::cout << '\n';
    }
    
    /**
     * Reads codemaker's answer.
     * 
     * @param b int to read number of pegs correct in color and position into
     * @param w int to read number or pegs correct in color only into
     * 
     * @returns `true` if the answer consists of exactly 2 integers in a
     * single line, `false` otherwise
     */
    bool read_answer(int &b, int &w) {
        std::vector<int> input;

        if (!read_n_ints(input, 2)) {
            return false;
        }

        b = input[0];
        w = input[1];
    
        return true;
    }

    /**
     * Validates codemaker's answer.
     * 
     * @param b number of pegs correct in color and position
     * @param w number of pegs correct in color only
     * @param n number of pegs
     * 
     * Answer is considered valid if 0 ≤ `b` ≤ `n`, 0 ≤ `w` ≤ `n` and
     * `b + w` ≤ `n`.
     * 
     * @returns `true` if the answer is valid, `false` otherwise
     */
    bool validate_answer(const int b, const int w, const int n) {
        return std::min(b, w) >= 0 && std::max(b, w) <= n && (b + w) <= n;
    }

    /**
     * Reads and validates codebreaker's answer.
     * 
     * See `read_answer()` and `validate_answer()` for more details.
     * 
     * @param b int to read number of pegs correct in color and position into
     * @param w int to read number of pegs correct in color only into
     * @param n number of pegs
     * 
     * @returns `read_answer(b, w)` AND `validate_answer(b, w, n)`
     */
    bool read_and_validate_answer(int &b, int &w, const int n) {
        return read_answer(b, w) && validate_answer(b, w, n);
    }

    /**
     * Validates game parameters.
     * 
     * @param k number of colors
     * @param n number of pegs
     * 
     * Parameters are considered valid if 2 ≤ `k` ≤ 256, 2 ≤ `n` ≤ 10 and
     * `kⁿ` ≤ 2²⁴.
     * 
     * @returns `true` if both parameters are valid, `false` otherwise
     */
    bool validate_parameters(const int k, const int n) {
        if (std::min(k, n) < 2 || k > 256 || n > 10) {
            return false;
        }

        std::uint32_t pow_k = 1;
        std::uint32_t limit = static_cast<std::uint32_t>(1) << 24;

        for (int i = 0; i < n; i++) {
            pow_k *= k;

            if (pow_k > limit) {
                return false;
            }
        }

        return true;
    }

    /**
     * Plays the game as a codebreaker.
     * 
     * @param k number of colors
     * @param n number of pegs
     * 
     * @returns `true` if both parameters are valid and the game ends without
     * any errors, `false` otherwise
     */
    bool play(const int k, const int n) {
        if (!validate_parameters(k, n)) {
            return false;
        }

        int b, w;
        std::vector<std::vector<int>> candidates;
        std::vector<int> empty_sequence;
        
        get_every_sequence(candidates, k, n, empty_sequence);

        do {
            std::vector<int> candidate = candidates.front();
    
            print_sequence(candidate);
            if (!read_and_validate_answer(b, w, n)) {
                return false;
            }
    
            // Removes all sequences that compare differently with
            // sequences.front(), as they cannot be the secret sequence.
            auto it = std::remove_if(
                candidates.begin(),
                candidates.end(),
                [&](const std::vector<int> &sequence) {
                    auto comp = compare_sequences(candidate, sequence);
                    return std::make_pair(b, w) != comp;
                }    
            );
    
            // Erases the "removed" elements.
            candidates.erase(it, candidates.end());

        } while(b < n && !candidates.empty());

        // If b < n, there are no candidates left - therefore, the secret
        // sequence does not exist.
        return b == n;
    }
}

namespace Codemaker {
    /**
     * Reads codebreaker's answer.
     * 
     * @param sequence vector to read colors of individual pegs into
     * @param n number of pegs
     * 
     * @returns `true` if the answer consists of exactly `n` integers in a
     * single line, `false` otherwise
     */
    bool read_answer(std::vector<int> &sequence, const int n) {
        return read_n_ints(sequence, n);
    }

    /**
     * Validates codebreaker's answer.
     * 
     * @param sequence colors of individual pegs
     * @param k number of colors
     * 
     * An answer is considered valid if each color ranges from 0 to `k - 1`.
     * 
     * @returns `true` if the answer is valid, `false` otherwise
     */
    bool validate_answer(const std::vector<int> &sequence, const int k) {
        for (std::size_t i = 0; i < sequence.size(); i++) {
            if (sequence[i] < 0 || sequence[i] >= k) {
                return false;
            }
        }

        return true;
    }

    /**
     * Reads and validates codebreaker's answer.
     * 
     * See `read_answer()` and `validate_answer()` for more details.
     * 
     * @param sequence vector to read colors of individual pegs into
     * @param k number of colors
     * @param n number of pegs
     * 
     * @returns `read_answer(sequence, n)` AND `validate_answer(sequence, k)`
     */
    bool read_and_validate_answer(
        std::vector<int> &sequence,
        const int k, const int n
    ) {
        return read_answer(sequence, n) && validate_answer(sequence, k);
    }

    /**
     * Validates game parameters.
     * 
     * @param k number of colors
     * @param secret colors of `n` individual pegs
     * 
     * Parameters are considered valid if 2 ≤ `k` ≤ 256, 2 ≤ `n` ≤ 10, 
     * `kⁿ` ≤ 2²⁴ and each color ranges from 0 to `k - 1`.
     * 
     * @returns `true` if both parameters are valid, `false` otherwise
     */
    bool validat_parameters(const int k, const std::vector<int> &secret) {
        int n = static_cast<int>(secret.size());

        // Checks if 2 ≤ k ≤ 256, 2 ≤ n ≤ 10 and kⁿ ≤ 2²⁴.
        if (!Codebreaker::validate_parameters(k, n)) {
            return false;
        }

        // Checks if each color ranges from 0 to (k - 1).
        return validate_answer(secret, k);
    }
    
    /**
     * Plays the game as a codemaker.
     * 
     * @param k number of colors
     * @param secret colors of individual pegs
     * 
     * @returns `true` if both parameters are valid and the game ends without
     * any errors, `false` otherwise
     */
    bool play(const int k, const std::vector<int> &secret) {
        if (!validat_parameters(k, secret)) {
            return false;
        }
        
        int b, w, n = static_cast<int>(secret.size());
        std::vector<int> sequence;

        do {
            if (!read_and_validate_answer(sequence, k, n)) {
                return false;
            }
    
            std::tie(b, w) = compare_sequences(sequence, secret);
            std::cout << b << ' ' << w << '\n';        
    
        } while(b < n);
    
        return true;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << ERROR_MESSAGE;
        return 1;
    }

    bool success = false;
    
    if (argc == 3) {
        int k, n;

        // Tries to parse game parameters and plays as a codebreaker.
        if (parse_int(k, argv[1]) && parse_int(n, argv[2])) {
            success = Codebreaker::play(k, n);
        }
    }
    else {
        int k;
        std::vector<int> secret;

        // Tries to parse game parameters and plays as a codemaker.
        if (parse_int(k, argv[1]) && parse_n_ints(secret, argv + 2, argc - 2)) {
            success = Codemaker::play(k, secret);
        }
    }

    if (!success) {
        std::cerr << ERROR_MESSAGE;
        return 1;
    }

    return 0;
}