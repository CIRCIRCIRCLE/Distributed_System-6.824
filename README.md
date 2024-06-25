# Distributed_System-6.824
Project source code from: https://github.com/tjumcw/6.824/tree/main

### Lab1: MapReduce
#### Key Points:
1. Map and Reduce Functions: Dynamically loaded and process data. Shuffle handles reading and aggregating intermediate data (one worker process), ReduceFunc aggregates the processed key-value pairs (all worker processes).
2. Worker Process: Executes map and reduce tasks. Example: Hello, hi, hello -> map: hello,1 hi,1 hello,1 -> reduce: hello,2 hi,1.
3. Master Process: Manages task allocation and status monitoring.
4. RPC Connection: Uses cppzmq RPC library.
5. Timeout and Retransmission: Timer starts when a task is assigned -> Timer thread waits for timeout -> Cancel the timer if the task is completed -> Reschedule the task if it times out.
