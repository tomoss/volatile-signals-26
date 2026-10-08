#ifndef TASK_HPP
#define TASK_HPP

#include <concepts>
#include <cstdint>
#include <functional>

template<typename T>
concept TaskLike = std::default_initializable<T> && requires(T& p_task, const char* p_name, std::function<void()> p_taskLoop, uint32_t p_priority) {
    { p_task.createAndStart(p_name, p_taskLoop) } -> std::convertible_to<bool>;
    { p_task.createAndStart(p_name, p_taskLoop, p_priority) } -> std::convertible_to<bool>;
    { p_task.running() } -> std::convertible_to<bool>;
};

#endif // TASK_HPP
