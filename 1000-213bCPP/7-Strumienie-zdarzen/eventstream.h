#ifndef EVENTSTREAM_H
#define EVENTSTREAM_H

#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>
#include <type_traits>
#include <utility>

namespace eventstream {
    enum class control {
        continue_,
        stop
    };

    auto emit(auto x) {
        return [=](auto observer, auto state) {
            return observer(x, state).first;
        };
    }

    auto generate(auto init, auto step) {
        return [=, init_store = std::optional{init}, was_invoked = false](
            auto observer,
            auto state
        ) mutable {
            // Emits `init` event, then `step(init)`, `step(step(init))`, etc.
            auto feed = [=, &init_store](
                this auto& self,
                auto event,
                auto state_
            ) -> decltype(state_) {
                const auto [next_state, action] = observer(event, state_);

                // Return if stopped...
                if (action == control::stop) {
                    return next_state;
                }

                if (const auto opt = step(event); opt.has_value()) {
                    init_store.emplace(opt.value());
                }
                else {
                    init_store = std::nullopt;
                }
                
                // ...or if the next event does not have a value.
                return !init_store.has_value()
                    ? next_state
                    : self(init_store.value(), next_state);
            };

            // Dummy step to 'continue' the previous invocation.
            if (was_invoked) {
                if (init_store.has_value()) {
                    const auto opt = step(init_store.value());
                    
                    if (opt.has_value()) {
                        init_store = opt;
                        return feed(opt.value(), state);
                    }
                }

                return state;
            }

            was_invoked = true;
            return feed(init_store.value(), state);
        };
    }

    inline auto counter() {
        static const auto step = [](int x) -> std::optional<int> {
            return x == std::numeric_limits<int>::max()
                ? std::numeric_limits<int>::min()
                : x + 1;
        };

        return generate(1, step);
    }

    auto map(auto f, auto s) {
        return [=](auto observer, auto state) mutable {
            // Emits `f(e)` for each event `e`.
            const auto map_observer = [=](auto event, auto state_) mutable {
                return observer(f(event), state_);
            };

            return s(map_observer, state);
        };
    }

    auto map(auto f) {
        return [=](auto s) {
            return map(f, s);
        };
    }

    auto filter(auto pred, auto s) {
        return [=](auto observer, auto state) mutable {
            // Emits events `e` for which `pred(e)` returns true.
            const auto filter_observer = [=](auto event, auto state_) mutable {
                return pred(event)
                    ? observer(event, state_)
                    : std::pair{state_, control::continue_};
            };

            return s(filter_observer, state);
        };
    }

    auto filter(auto pred) {
        return [=](auto s) {
            return filter(pred, s);
        };
    }

    auto take(std::size_t n, auto s) {
        return [=](auto observer, auto state) mutable {
            std::size_t remaining = n;

            // Emits at most the first `n` events.
            const auto take_observer = [=, &remaining](
                auto event,
                auto state_
            ) mutable {
                return (--remaining == 0)
                    ? std::pair{observer(event, state_).first, control::stop}
                    : observer(event, state_);
            };

            return (remaining == 0) ? state : s(take_observer, state);
        };
    }

    inline auto take(std::size_t n) {
        return [=](auto s) {
            return take(n, s);
        };
    }

    auto flatten(auto ss) {
        return [=](auto observer, auto state) mutable {
            bool stopped = false;

            // Emits events while intercepting the STOP signal.
            const auto inner_observer = [=, &stopped](
                auto event,
                auto state_
            ) mutable {
                const auto result = observer(event, state_);

                if (result.second == control::stop) {
                    stopped = true;
                }

                return result;
            };

            // Emits all events from the `event` stream.
            const auto outer_observer = [=, &stopped](
                auto event,
                auto state_
            ) mutable {
                return std::pair{
                    event(inner_observer, state_),
                    stopped ? control::stop : control::continue_
                };
            };

            return ss(outer_observer, state);
        };
    }

    inline auto flatten() {
        return [=](auto ss) {
            return flatten(ss);
        };
    }

    namespace detail {
        template <typename T>
        struct function_traits;

        // Specialization for std::function.
        template <typename R, typename... Args>
        struct function_traits<std::function<R(Args...)>> {
            using return_type = R;
            using args_tuple = std::tuple<std::decay_t<Args>...>;
        };

        template <typename F>
        class memoize_wrapper {
            private:
                using traits = detail::function_traits<F>;

                using return_type = typename traits::return_type;
                using args_tuple = typename traits::args_tuple;

                F f;
                mutable std::map<args_tuple, return_type> cache;

            public:
                memoize_wrapper(F f) : f{f} {}

                template <typename... Args>
                return_type operator()(Args... args) const {
                    args_tuple args_tp{args...};

                    // Lookup for the (possibly) cached invocation and result.
                    if (auto it = cache.find(args_tp); it != cache.end()) {
                        return it->second;
                    }

                    return_type result = f(args...);
                    return cache.emplace(args_tp, result).first->second;
                }
        };

        // Partial specialization for void return type.
        template <typename... Args>
        class memoize_wrapper<std::function<void(Args...)>> {
            private:
                using args_tuple = std::tuple<std::decay_t<Args>...>;

                std::function<void(Args...)> f;
                mutable std::set<args_tuple> cache;

            public:
                memoize_wrapper(std::function<void(Args...)> f) : f{f} {}

                void operator()(Args... args) const {
                    args_tuple args_tp{args...};

                    // Lookup for the (possibly) cached invocation.
                    if (cache.contains(args_tp)) {
                        return;
                    }

                    f(args...);
                    cache.emplace(args_tp);
                }
        };

    } /* namespace detail */

    auto memoize(auto f) {
        return detail::memoize_wrapper<decltype(std::function{f})>(
            std::function{f}
        );
    }

    auto operator|(auto stream, auto transformer) {
        return transformer(stream);
    }

    auto tap(auto side_effect) {
        return [=](auto s) {
            return [=](auto observer, auto state) mutable {
                // Emits events while also emitting them to `side_effect()`.
                const auto tap_observer = [=](auto event, auto state_) mutable {
                    side_effect(event);
                    return observer(event, state_);
                };

                return s(tap_observer, state);
            };
        };
    }

} /* namespace eventstream */

#endif /* EVENTSTREAM_H */
