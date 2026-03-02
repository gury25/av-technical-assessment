## Context

Your task is to implement a simplified, multi-threaded Task processing system. This system is like a pipeline where tasks are generated, processed asynchronously, and then transmitted over a communication channel (printed to stdout in our case). This specification is intended to be vague in nature, we expect you make assumptions and justify them in an assumptions.txt file.

The system has three main components, each running in its own thread:

1.  **Task Generator:** Produces various types of tasks and places them into a shared queue.
2.  **Task Processor:** Consumes tasks from the queue, performs a specific operation, and places the results into a second shared queue.
3.  **Data Transmitter:** Consumes the processed data, bit packs it into a fixed size buffer, and transmits it by printing the buffers contents.

The entire system must operate concurrently and be correctly synchronized to ensure thread safety and data integrity. The simulation should run for a fixed duration and then shut down **gracefully**.

## Task

Implement the following components and integrate them into a complete, runnable program. You may modify the provided class structures as needed, but the core requirements below must be met.

### Task 1: Task Representation

Create a polymorphic base class `ITask`.

-   Implement two derived classes: `SimpleTask` and `ComplexTask`.
-   `SimpleTask` should represent a basic operation. It should contain a `float` value and its `process()` method should simply multiply this value by $2.0$.
-   `ComplexTask` should represent a more involved operation. It should contain a `std::vector<int>` and its `process()` method should calculate the sum of all elements in the vector.

All tasks must be processed in a generic way by the `TaskProcessor` thread, such that give a base class has been passed, it should call the appropriate process function.

### Task 2: Concurrency and Shared Queues

Using the available syncronization utilties available in C++ implement the `ThreadSafeQueue`, the functions are stubbed within the class for you.

-   The Task Generator thread should push `std::unique_ptr<ITask>` objects into a thread safe queue (`task_queue_`).
-   The Task Processor thread should pop from `task_queue_`, call the `process()` method, and then push the processed task into a second thread safe queue (`processed_queue_`).
-   The Data Transmitter thread should pop from `processed_queue_`.

**Important:** All threads must be able to shut down gracefully after a specific duration. You should implement a clean shutdown mechanism using the given `std::atomic<bool>` flag.

### Task 3: Data Serialization (Bitpacking)

The Data Transmitter thread must bitpack the processed data into a buffer of 8 bytes (`uint8_t[8]`).

#### Bitpacking Specification:

Your `uint8_t[8]` buffer must contain the following data fields in the specified bit order. Use bitwise operations to pack the data.

-   **Byte 0 (MSB):**
    -   Bits 7-6: `TaskType` (00 for SimpleTask, 01 for ComplexTask).
    -   Bits 5-0: `Padding`.
-   **Bytes 1-4:** Processed value (a `float` or `int`, depending on the task type).
    -   **Note:** You must handle the packing of the `float` or `int` value into 4 bytes.
-   **Bytes 5-7 (LSB):** A simple timestamp (`uint32_t`) representing the time of transmission. Only the lower 24 bits of the timestamp should be packed.

### Verification

The correctness of your solution will be checked by verifying the output. Your program must print the hexadecimal representation of the 8-byte transmission buffer for each transmitted packet. The output must be correctly packed as per the specification.

#### Example Test Case:

If a `SimpleTask` with an initial value of $10.0$ is processed, its processed value will be $20.0$. If this is transmitted at timestamp $100$, the packed buffer should represent `TaskType=00`, `processed_value=20.0`, and `timestamp=100`.

A sample output for this specific case would be a single line of hexadecimal values, e.g.:

```
0x00 0x41 0xa0 0x00 0x00 0x00 0x00 0x64
```

You are free to choose the specific values for your simulation. Please use the provided `main` function structure as a starting point. Your implementation should be in the empty function bodies.

### Task 4: Unit Testing

To ensure the correctness and robustness of your implementation, you are required to write unit tests for the key parts of the system using the Google Test framework (GTest).

Write a suite of tests that verify the functionality of your `ITask` implementations and the bitpacking logic.

Before you can attempt this task you will need to update the `CMakeLists.txt` to include GTest and create a test executable that runs your tests from your `Question-2.test.cc` file.

### Task 5: Assumptions

Justify any assumptions you made during your implementation in a file named `assumptions.txt`.
