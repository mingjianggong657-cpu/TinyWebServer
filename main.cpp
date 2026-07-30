#include <iostream>
#include <chrono>
#include "threadpool.h"

int main() {
    ThreadPool pool(4); // 创建 4 个线程

    // 提交 8 个打印任务
    for (int i = 0; i < 8; ++i) {
        pool.addTask([i] {
            std::cout << "Task " << i << " is running on thread " 
                      << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "All tasks finished!" << std::endl;
    return 0;
}
