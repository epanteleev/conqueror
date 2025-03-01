#include <iostream>

#include "SPSCBoundedQueue.h"

int main() {
    conq::SPSCBoundedQueue<int, 16> queue;
    queue.try_push(1);
    queue.try_push(2);

    auto val = queue.try_pop();
    if (val.has_value()) {
        std::cout << "Value: " << val.value() << std::endl;
    }
    return 0;
}
