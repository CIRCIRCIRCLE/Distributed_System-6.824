# Distributed_System-6.824
项目源代码来自： https://github.com/tjumcw/6.824/tree/main

### Lab1: MapReduce
#### Key Points:
1. Map和Reduce函数，动态加载并处理数据： Shuffle处理读取并聚合中间件（one worker process）, ReduceFunc聚合处理后的键值（all worker processes）
2. Worker进程，执行map, reduce任务  Hello, hi, hello -> map: hello,1 hi,1 hello,1 -> reduce: hello,2 hi,1
3. Master进程，管理任务的分配与状态监控
4. RPC连接: cppzmq rpc库
5. 超时重传：任务分配时启动计时器 -》 计时器线程等待超时 -》 任务完成前取消计时器 -》 任务超时重分配

```cpp
编译程序
cd mapreduce
1. 编译 map_reduceFun.cpp 并生成共享库libmrFunc.so
g++ -fpic -c map_reduceFun.cpp
g++ -shared map_reduceFun.o -o libmrFunc.so
2. 编译worker.cpp
g++ worker.cpp -ldl -o worker -I./buttonrpc-master -lzmq -pthread
3. 编译master.cpp
g++ master.cpp -o master -I./buttonrpc-master -lzmq -pthread
```
```cpp
运行
./master  files/pg*.txt
set another terminal, run the following
LD_LIBRARY_PATH=. ./worker
```
