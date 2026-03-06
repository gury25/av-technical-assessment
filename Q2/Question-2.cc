#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include <iomanip>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <cstring>
#include <ctime>

template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }
    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    T pop_for_shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return nullptr;
        }
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }
};

class ITask {
public:
    virtual ~ITask() = default;
    virtual void process() = 0;
    virtual float getProcessedValue() const = 0;
    virtual uint8_t getTaskType() const = 0;
};

class SimpleTask : public ITask {
private:
    float value_;
    float processed_value_;
public:
    explicit SimpleTask(float val) : value_(val), processed_value_(0.0f) {}
    void process() override {
        processed_value_ = value_ * 2.0f;
    }
    float getProcessedValue() const override {
        return processed_value_;
    }
    uint8_t getTaskType() const override {
        return 0;
    }
};

class ComplexTask : public ITask {
private:
    std::vector<int> nums_;
    int processed_value_;
public:
    explicit ComplexTask(std::vector<int> nums) : nums_(nums), processed_value_(0) {}
    void process() override {
        processed_value_ = 0;
        for (int num : nums_) {
            processed_value_ += num;
        }
    }
    float getProcessedValue() const override {
        return static_cast<float>(processed_value_);
    }
    uint8_t getTaskType() const override {
        return 1;
    }
};


class TaskGenerator {
private:
    ThreadSafeQueue<std::unique_ptr<ITask>>& task_queue_;
    std::atomic<bool>& shutdown_;
public:
    TaskGenerator(ThreadSafeQueue<std::unique_ptr<ITask>>& queue, std::atomic<bool>& shutdown)
        : task_queue_(queue), shutdown_(shutdown) {}
    void run() {
        int counter = 0;
        while (!shutdown_) {
            if (counter % 2 == 0) {
                float value = 5.0f + (counter * 0.5f);
                task_queue_.push(std::make_unique<SimpleTask>(value));
            } else {
                std::vector<int> nums = {1, 2, 3, 4, 5};
                task_queue_.push(std::make_unique<ComplexTask>(nums));
            }
            counter++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};

class TaskProcessor {
private:
    ThreadSafeQueue<std::unique_ptr<ITask>>& task_queue_;
    ThreadSafeQueue<std::unique_ptr<ITask>>& processed_queue_;
    std::atomic<bool>& shutdown_;
public:
    TaskProcessor(ThreadSafeQueue<std::unique_ptr<ITask>>& t_queue, ThreadSafeQueue<std::unique_ptr<ITask>>& p_queue, std::atomic<bool>& shutdown)
        : task_queue_(t_queue), processed_queue_(p_queue), shutdown_(shutdown) {}
    void run() {
        while (!shutdown_) {
            std::unique_ptr<ITask> task = task_queue_.pop();
            if (task) {
                task->process();
                processed_queue_.push(std::move(task));
            }
            if (shutdown_ && task_queue_.size() == 0) {
                break;
            }
        }
    }
};

class PacketTransmitter {
private:
    ThreadSafeQueue<std::unique_ptr<ITask>>& processed_queue_;
    std::atomic<bool>& shutdown_;
public:
    PacketTransmitter(ThreadSafeQueue<std::unique_ptr<ITask>>& queue, std::atomic<bool>& shutdown)
        : processed_queue_(queue), shutdown_(shutdown) {}
    void run() {
        while (!shutdown_) {
            std::unique_ptr<ITask> task = processed_queue_.pop();
            if (task) {
                transmit(task, std::cout);
            }
            if (shutdown_ && processed_queue_.size() == 0) {
                break;
            }
        }
    }
    void transmit(const std::unique_ptr<ITask>& data, std::ostream& os) {
        uint8_t buffer[8] = {0};

        uint8_t task_type = data->getTaskType();

        buffer[0] = (task_type & 0x03) << 6; 

        float processed_value = data->getProcessedValue();
        uint32_t float_bits;
        std::memcpy(&float_bits, &processed_value, sizeof(float));
        
        buffer[1] = (float_bits >> 24) & 0xFF;
        buffer[2] = (float_bits >> 16) & 0xFF;
        buffer[3] = (float_bits >> 8) & 0xFF;
        buffer[4] = float_bits & 0xFF;

        uint32_t timestamp = static_cast<uint32_t>(std::time(nullptr)) & 0xFFFFFF;
        buffer[5] = (timestamp >> 16) & 0xFF;
        buffer[6] = (timestamp >> 8) & 0xFF;
        buffer[7] = timestamp & 0xFF;

        os << "Packet: ";
        for (int i = 0; i < 8; ++i) {
            os << "0x" << std::setw(2) << std::setfill('0') << std::hex << (int)buffer[i] << " ";
        }
        os << std::dec << std::endl;
    }
};

int main() {
    std::cout << "Starting the data generation pipeline" << std::endl;

    std::atomic<bool> shutdown_flag{false};

    ThreadSafeQueue<std::unique_ptr<ITask>> task_queue;
    ThreadSafeQueue<std::unique_ptr<ITask>> processed_queue;

    TaskGenerator generator(task_queue, shutdown_flag);
    TaskProcessor processor(task_queue, processed_queue, shutdown_flag);
    PacketTransmitter transmitter(processed_queue, shutdown_flag);

    std::thread generator_thread(&TaskGenerator::run, &generator);
    std::thread processor_thread(&TaskProcessor::run, &processor);
    std::thread transmitter_thread(&PacketTransmitter::run, &transmitter, std::ref(std::cout));

    std::this_thread::sleep_for(std::chrono::seconds(10));

    shutdown_flag = true;

    generator_thread.join();
    processor_thread.join();
    transmitter_thread.join();

    std::cout << "Data Gen pipeline Finished." << std::endl;

    return 0;
}
