#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <concepts>

template<typename TQueue, typename T>
concept QueueLike = std::default_initializable<TQueue> && requires(TQueue& p_queue, const T& p_in, T& p_out) {
    { p_queue.init() } -> std::convertible_to<bool>;
    { p_queue.send(p_in) } -> std::convertible_to<bool>;
    { p_queue.receive(p_out) } -> std::convertible_to<bool>;
};

#endif // QUEUE_HPP
