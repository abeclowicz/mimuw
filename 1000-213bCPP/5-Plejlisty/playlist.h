#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

namespace cxx {

    template <typename T, typename P>
    class playlist {
        // In the following time complexities n denotes
        // the number of records stored in the playlist.

        private:
            struct playlist_impl {
                // Forward declare the struct to avoid
                // circular dependencies relating to iterator types.
                struct Entry;

                using play_tracks_t = std::list<Entry>;
                using play_iter_t = typename play_tracks_t::iterator;

                /**
                 * Defines the '<' operator for the `play_tracks_t::iterator`
                 * type which allows to store iterators in a `std::set`.
                 * 
                 * Iterators are compared by their memory addresses.
                 */
                struct IterCmp {
                    bool operator()(const play_iter_t &lhs,
                                    const play_iter_t &rhs) const {
                        return &(*lhs) < &(*rhs);
                    }
                };

                using iter_set_t = std::set<play_iter_t, IterCmp>;

                /**
                 * Defines the transparent comparator for the
                 * `std::map<T, iter_set_t>` which enables heterogeneous lookup.
                 */
                struct TrackCmp {
                    using is_transparent = void;

                    bool operator()(const T &lhs, const T &rhs) const {
                        return lhs < rhs;
                    }

                    bool operator()(const T *lhs, const T &rhs) const {
                        return *lhs < rhs;
                    }

                    bool operator()(const T &lhs, const T *rhs) const {
                        return lhs < *rhs;
                    }
                };

                using sorted_tracks_t = std::map<const T, iter_set_t, TrackCmp>;
                using sorted_iter_t = typename sorted_tracks_t::const_iterator;

                // Define the struct now that all iterator types are known.
                // For each set of params we keep:
                // - an iterator to the <K, V> pair where the track is stored
                // - an iterator to the V element (iterator to this Entry)
                struct Entry {
                    P params;
                    typename sorted_tracks_t::iterator map_iter;
                    typename iter_set_t::iterator set_iter;
                };

                play_tracks_t play_tracks;
                sorted_tracks_t sorted_tracks;

                bool shareable = true;

                playlist_impl() = default;
                ~playlist_impl() noexcept = default;
            };

            mutable std::shared_ptr<playlist_impl> data;

            /**
             * Initializes `data` if it is null (e.g. after a move operation).
             * 
             * Time complexity: `O(1)`
             */
            void check_null_data() const {
                if (!data) {
                    data = std::make_shared<playlist_impl>();
                }
            }

            /**
             * Deep-copies shared playlist data.
             * 
             * Time complexity: `O(n log n)`
             */
            void copy_on_write(bool shareable = true);

        public:
            /**
             * Creates an empty playlist.
             * 
             * Time complexity: `O(1)`
             */
            playlist() : data(std::make_shared<playlist_impl>()) {}


            /**
             * Copy constructor.
             * 
             * Deep copies `other` if it is not shareable.
             * 
             * Time complexity: `O(1)`, `O(n log n)` if copied
             */
            playlist(playlist const &other) : data(other.data) {
                if (data && !data->shareable) {
                    copy_on_write();
                }
            }

            /**
             * Move constructor.
             * 
             * Leaves the moved-from `other` playlist in a
             * 'valid state' by setting its data to `nullptr`.
             * 
             * Time complexity: `O(1)`
             */
            playlist(playlist &&other) noexcept : data(std::move(other.data)) {
                other.data = nullptr;
            }

            /// Time complexity: `O(1)`
            ~playlist() noexcept = default;

            /// Time complexity: `O(1)`
            playlist & operator=(playlist other) noexcept {
                data = std::move(other.data);
                return *this;
            }

            /**
             * Iterator that allows traversal over all
             * records in order of their respective insertions.
             * 
             * All methods are `O(1)` and `noexcept`.
             */
            class play_iterator : private playlist_impl::play_iter_t {
                friend class playlist;
                using Base = typename playlist_impl::play_iter_t;

                public:
                    using iterator_category = std::forward_iterator_tag;
                    using value_type = Base::value_type;
                    using difference_type = Base::difference_type;
                    using reference = Base::reference;

                    play_iterator() noexcept = default;

                    play_iterator(const Base &other) noexcept : Base(other) {}

                    play_iterator(const play_iterator &other)
                    noexcept = default;

                    play_iterator & operator=(const play_iterator &other)
                    noexcept = default;

                    bool operator==(const play_iterator &other) const noexcept {
                        return static_cast<const Base&>(*this) ==
                               static_cast<const Base&>(other);
                    }

                    bool operator!=(const play_iterator &other) const noexcept {
                        return static_cast<const Base&>(*this) !=
                               static_cast<const Base&>(other);
                    }

                    play_iterator & operator++() noexcept {
                        Base::operator++();
                        return *this;
                    }

                    play_iterator operator++(int) noexcept  {
                        play_iterator iter = *this;
                        Base::operator++();
                        return iter;
                    }
            };

            /**
             * Returns a `play_iterator` to the first entry.
             * 
             * Time complexity: `O(1)`
             */
            play_iterator play_begin() const {
                check_null_data();
                return { data->play_tracks.begin() };
            }

            /**
             * Returns a `play_iterator` past the last entry.
             * 
             * Time complexity: `O(1)`
             */
            play_iterator play_end() const {
                check_null_data();
                return { data->play_tracks.end() };
            }

            /**
             * Iterator that allows traversal over all
             * tracks in order of their default ordering.
             * 
             * All methods are `O(1)` and `noexcept`.
             */
            class sorted_iterator : private playlist_impl::sorted_iter_t {
                friend class playlist;
                using Base = typename playlist_impl::sorted_iter_t;

                public:
                    using iterator_category = std::forward_iterator_tag;
                    using value_type = Base::value_type;
                    using difference_type = Base::difference_type;
                    using reference = Base::reference;

                    sorted_iterator() noexcept = default;

                    sorted_iterator(const Base &other) noexcept : Base(other) {}
                    
                    sorted_iterator(const sorted_iterator &other)
                    noexcept = default;

                    sorted_iterator & operator=(const sorted_iterator &other)
                    noexcept = default;

                    bool operator==(const sorted_iterator &other) const
                    noexcept {
                        return static_cast<const Base&>(*this) ==
                               static_cast<const Base&>(other);
                    }

                    bool operator!=(const sorted_iterator &other) const
                    noexcept {
                        return static_cast<const Base&>(*this) !=
                               static_cast<const Base&>(other);
                    }

                    sorted_iterator & operator++() noexcept {
                        Base::operator++();
                        return *this;
                    }

                    sorted_iterator operator++(int) noexcept {
                        sorted_iterator iter = *this;
                        Base::operator++();
                        return iter;
                    }
            };

            /**
             * Returns a `sorted_iterator` to the first entry.
             * 
             * Time complexity: `O(1)`
             */
            sorted_iterator sorted_begin() const {
                check_null_data();
                return { data->sorted_tracks.begin() };
            }

            /**
             * Returns a `sorted_iterator` past the last entry.
             * 
             * Time complexity: `O(1)`
             */
            sorted_iterator sorted_end() const {
                check_null_data();
                return { data->sorted_tracks.end() };
            }

            /**
             * Adds a new (`track`, `params`) record to the playlist.
             * 
             * Time complexity: `O(log n)`, `O(n log n)` if copied
             */
            void push_back(T const &track, P const &params) {
                std::weak_ptr<playlist_impl> original_data = data;
                check_null_data();
                copy_on_write();

                try {
                // Try to insert the track into the sorted map.
                auto [sorted_iter, inserted] =
                    data->sorted_tracks.try_emplace(track);

                try {
                    // Append new entry to the play_tracks list.
                    // We will fix default 'set_iter' in a moment.
                    auto play_iter = data->play_tracks.insert(play_end(),
                        { params, sorted_iter, {} }
                    );

                    try {
                        // Insert the iterator into the sorted map.
                        auto res = sorted_iter->second.insert(play_iter);

                        // Store the set iterator back into list entry.
                        play_iter->set_iter = res.first;
                    }
                    catch (...) {
                        // Rollback play_tracks insertion.
                        data->play_tracks.erase(play_iter);

                        throw;
                    }
                }
                catch (...) {
                    // Rollback sorted map insertion (if there was one).
                    if (inserted) {
                        data->sorted_tracks.erase(sorted_iter);
                    }

                    throw;
                }
                }
                catch (...) {
                    if (auto p = original_data.lock()) {
                        data = p;
                    }

                    throw;
                }
            }

            /**
             * Removes the first record from the playlist.
             * 
             * Throws `std::out_of_range` if the playlist is empty.
             * 
             * Time complexity: `O(1)`, `O(n log n)` if copied
             */
            void pop_front() {
                if (!data || data->play_tracks.empty()) {
                    throw std::out_of_range("pop_front, playlist empty");
                }

                copy_on_write();

                // Get the first entry from the play_tracks list.
                auto &entry = *(data->play_tracks.begin());

                // Remove the entry from the set.
                // Since we pass an iterator to self, the erase runs in O(1).
                entry.map_iter->second.erase(entry.set_iter);

                // Don't keep any zombie keys.
                if (entry.map_iter->second.empty()) {
                    data->sorted_tracks.erase(entry.map_iter);
                }

                data->play_tracks.pop_front();
            }

            /**
             * Returns a reference to the first record in the playlist.
             * 
             * Throws `std::out_of_range` if the playlist is empty.
             * 
             * Time complexity: `O(1)`
             */
            const std::pair<T const &, P const &> front() const {
                if (!data || data->play_tracks.empty()) {
                    throw std::out_of_range("front, playlist empty");
                }

                // Get the first entry from the play_tracks list.
                auto &entry = *(data->play_tracks.begin());

                return {entry.map_iter->first, entry.params};
            }

            /**
             * Removes all records with a given `track` from the playlist.
             * 
             * Throws `std::invalid_argument` if no such record is found.
             * 
             * Time complexity: `O(log n + k)`,
             * where `k` denotes the number of removed records
             */
            void remove(T const &track) {
                if (!data) {
                    throw std::invalid_argument("remove, unknown track");
                }

                auto sorted_iter = data->sorted_tracks.find(track);

                if (sorted_iter == sorted_end()) {
                    throw std::invalid_argument("remove, unknown track");
                }

                std::weak_ptr<playlist_impl> original_data = data;
                copy_on_write();
                
                try {
                // Re-find in case copy_on_write() invalidated iterators.
                sorted_iter = data->sorted_tracks.find(track);

                // Remove all occurrences of the track in play_tracks.
                for (auto &play_iter : sorted_iter->second) {
                    data->play_tracks.erase(play_iter);
                }

                // Erase the track from the sorted map
                // now that all its occurrences are gone.
                data->sorted_tracks.erase(sorted_iter);
                }
                catch (...) {
                    if (auto p = original_data.lock()) {
                        data = p;
                    }

                    throw;
                }
            }

            /**
             * Removes all records from the playlist.
             * 
             * Time complexity: `O(n)`, `O(1)` if copied
             */
            void clear() noexcept {
                // We do not call check_null_data() or copy_on_write()
                // here to avoid initializing/copying data just to destroy it.

                if (!data || !data.unique()) {
                    data = nullptr;
                }
                else {
                    data->play_tracks.clear();
                    data->sorted_tracks.clear();
                }
            }

            /**
             * Returns the number of records in the playlist.
             * 
             * Time complexity: `O(1)`
             */
            size_t size() const noexcept {
                // Since we avoid calling check_null_data()
                // here, this method can be noexcept.

                return data ? data->play_tracks.size() : 0;
            }

            /**
             * Returns a reference to the record that `it` points to.
             * 
             * Time complexity: `O(1)`
             */
            const std::pair<T const &, P const &>
            play(play_iterator const &it) const {
                const auto& base_it = static_cast<play_iterator::Base>(it);
                return {base_it->map_iter->first, base_it->params};
            }

            /**
             * Returns a reference to the track from the record that `it`
             * points to and the number of its occurrences in the playlist.
             * 
             * Time complexity: `O(1)`
             */
            const std::pair<T const &, size_t>
            pay(sorted_iterator const &it) const {
                const auto& base_it = static_cast<sorted_iterator::Base>(it);
                return {base_it->first, base_it->second.size()};
            }

            /**
             * Returns a reference to the params
             * of the record that `it` points to.
             * 
             * Time complexity: `O(1)`, `O(n log n)` if copied
             */
            P & params(play_iterator const &it) {
                // We assume data is not null if there are any valid iterators.

                const auto& base_it = static_cast<play_iterator::Base>(it);

                if (!data.unique()) {
                    // Save the position of the original iterator.
                    auto dist = std::distance(
                        data->play_tracks.begin(), 
                        base_it
                    );

                    copy_on_write(false);

                    // Now we need a new iterator, since the
                    // original still points to the previous data.
                    auto new_it = data->play_tracks.begin();
                    std::advance(new_it, dist);

                    return new_it->params;
                }

                // At that point, data is guaranteed to be unique.
                // Simply disable sharing, as the reference may mutate.
                data->shareable = false;

                return base_it->params;
            }

            /**
             * Returns a const reference to the params
             * of the record that `it` points to.
             * 
             * Time complexity: `O(1)`
             */
            const P & params(play_iterator const &it) const noexcept {
                const auto& base_it = static_cast<play_iterator::Base>(it);
                return base_it->params;
            }
        };

        template <typename T, typename P>
        void playlist<T, P>::copy_on_write(bool shareable) {
            check_null_data();

            if (!data.unique()) {
                playlist plist;

                // Copy all tracks into a new playlist instance.
                for (auto it = play_begin(); it != play_end(); ++it) {
                    plist.push_back(play(it).first, play(it).second);
                }

                // Replace current data with the new, unique copy.
                data = plist.data;
            }

            data->shareable = shareable;
        }

} /* namespace cxx */

#endif /* PLAYLIST_H */
