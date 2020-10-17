#include <iostream>
#include <thread>

#include <satella/satella.hpp>

satella::post_func_t create_worker_thread(const satella::cancellation_token &token)
{
    // Create job_queue
    auto job_queue = std::make_shared<satella::single_consumer_queue<satella::job_func_t>>();

    // Create worker thread
    std::thread worker([token, job_queue] {
        std::cout << "worker thread: " << std::this_thread::get_id() << std::endl;
        try {
            for (;;) {
                // pop and execute job function
                // note: pop() blocks threads until push() is executed
                job_queue->pop(token)();
            }
        } catch (satella::cancelled_error) {
        }
    });
    worker.detach();

    // Create producer function
    return [job_queue](satella::job_func_t &&job) {
        job_queue->push(std::move(job));
    };
}

int main()
{
    std::cout << "main thread: " << std::this_thread::get_id() << std::endl;

    satella::cancellation_token_source cts;

    auto producer = create_worker_thread(cts.get_token());

    // Create task1 that returns the value of int
    auto task1 = satella::make_task(producer, [] {
        std::cout << "start task1 thread: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "end task1" << std::endl;
        return 5;
    });
    // task1 is task<int>
    static_assert(std::is_same<decltype(task1), satella::task<int>>::value);

    // Create task2 that receives the result of task1 and returns the value of double
    auto task2 = std::move(task1).then([](auto &&result) {
        // result is int
        static_assert(std::is_same<decltype(result), int&&>::value);

        std::cout << "start task2 thread: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "end task2" << std::endl;
        if (result == 0) {
            throw std::invalid_argument("result == 0");
        }
        return (double)result / 2;
    });
    // task2 is task<double>
    static_assert(std::is_same<decltype(task2), satella::task<double>>::value);

    // Create task3 that receives the result of task2 and returns the value of float
    // note: If an exception occurs in task1, task2 will not be executed but task3 will be executed
    auto task3 = std::move(task2).then_catch([](auto &&result) {
        // result is variant<std::exception_ptr, double>
        static_assert(std::is_same<decltype(result), satella::variant<std::exception_ptr, double>&&>::value);

        // catch task2 (or task1) exception
        double val;
        if (satella::index(result) == 0) {
            try {
                std::rethrow_exception(satella::get<0>(result));
            } catch (const std::invalid_argument&) {
                val = 0;
            }
        } else {
            val = satella::get<1>(result);
        }

        std::cout << "start task3 thread: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "end task3" << std::endl;
        return (float)val * 3;
    });
    // task3 is task<float>
    static_assert(std::is_same<decltype(task3), satella::task<float>>::value);

    // Create producer function 2
    auto producer2 = create_worker_thread(cts.get_token());

    // Create task4 to be executed by worker thread 2
    auto task4 = std::move(task3).then(producer2, [](auto &&result) {
        // result is float
        static_assert(std::is_same<decltype(result), float&&>::value);

        std::cout << "start task4 thread: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "end task4" << std::endl;
        return result * 2;
    });
    // task4 is task<float>
    static_assert(std::is_same<decltype(task4), satella::task<float>>::value);

    // Can be assigned to a convertible type task
    satella::task<double> task5 = std::move(task4);

    double result;
    try {
        // Wait for the result of task5
        // note: The main thread is not blocked by the time you reach this point
        //       All tasks are executed in worker thread
        auto val = std::move(task5).get();

        // val is float
        static_assert(std::is_same<decltype(val), double>::value);

        result = val;
    } catch (...) {
        // Handles exceptions that were not caught in task3 or that occurred in task4
        result = -1;
    }

    // result == 15
    std::cout << "result: " << result << std::endl;

    cts.cancel();
}
