# Distributed_System-6.824
Project source code from: https://github.com/tjumcw/6.824/tree/main

### Lab1: MapReduce
#### Key Points:
1. Map and Reduce Functions: Dynamically loaded and process data. Shuffle handles reading and aggregating intermediate data (one worker process), ReduceFunc aggregates the processed key-value pairs (all worker processes).
2. Worker Process: Executes map and reduce tasks. Example: Hello, hi, hello -> map: hello,1 hi,1 hello,1 -> reduce: hello,2 hi,1.
3. Master Process: Manages task allocation and status monitoring.
4. RPC Connection: Uses cppzmq RPC library.
5. Timeout and Retransmission: Timer starts when a task is assigned -> Timer thread waits for timeout -> Cancel the timer if the task is completed -> Reschedule the task if it times out.


### Lab2: RAFT
#### Key Points:
1. Leader Election:     
    __listenForVote Thread:__ Binds to `requestVote` to handle vote requests.    
    __Create electionLoop Thread:__ Implements the election logic during the election period, including initiating elections, requesting votes, and processing vote results.   

2. Log Replication:
    __listenForAppend Thread:__ Binds to `appendEntries` to handle log append requests.    
    __Create processEntriesLoop Thread:__ Performs log replication in a loop on each node to ensure log consistency.    

3. State Persistence:
    __Read Persistent State:__ Uses `readRaft` to call `deserialize()` to read and load the state from disk.
    __Save Persistent State:__ Uses `saveRaft` to call `serialize()` to serialize the current state and write it to disk.   

4. ApplyLogLoop:
    __Apply Log Entries:__ Applies committed but not yet applied log entries to the state machine, ensuring consistency between the state machine and the log.