## Lab1: MapReduce
### 目录结构：
```cpp
buttonrpc-master/: 包含ButtonRPC库文件。
files/: 输入文件目录，包含待处理的文本文件。
output/: 输出文件目录，用于存储处理结果。
map_reduceFun.cpp: 定义了Map和Reduce函数，进行数据处理。
worker.cpp: 定义了Worker进程的逻辑，包括如何处理Map和Reduce任务。
master.cpp: 定义了Master进程的逻辑，包括如何管理任务的分配和状态监控。
```

### 文件功能：
1. map_reduceFun.cpp: 定义了Map和Reduce函数。Map函数将输入文件内容拆分成键值对，Reduce函数对中间结果进行聚合。例如：
    - Map: 输入文本hello world hello -> ["hello", "world", "hello"] -> 输出 ["hello, 1", "world, 1", "hello, 1"]
    - Reduce: 输入 ["hello, 1", "hello, 1", "world, 1"] -> 输出 ["hello, 2", "world, 1"]

2. worker.cpp: Worker进程从Master请求任务，执行Map或Reduce操作，并返回结果。主要逻辑包括：接收任务, 执行Map或Reduce函数, 将结果写入输出文件, 通知Master任务完成

3. master.cpp: Master进程管理所有任务的分配和监控，包括任务的超时处理。主要逻辑包括：分配Map和Reduce任务, 监控任务状态处理, 任务超时和重传
