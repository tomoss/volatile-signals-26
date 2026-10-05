#ifndef TASK_HPP
#define TASK_HPP

#include <cstdint>
#include <functional>

class Task {
public:
    using TaskLoop = std::function<void()>;

    virtual ~Task() = default;

    virtual bool createAndStart(const char* p_name, TaskLoop p_taskLoop, uint32_t p_priority = 1, uint32_t p_stackSize = 4096) = 0;

    virtual bool running() const = 0;
};

#endif // TASK_HPP
