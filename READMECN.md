# Distributed_System-6.824
项目源代码来自： https://github.com/tjumcw/6.824/tree/main

### Lab1: MapReduce
#### Key Points:
1. Map和Reduce函数，动态加载并处理数据： Shuffle处理读取并聚合中间件（one worker process）, ReduceFunc聚合处理后的键值（all worker processes）
2. Worker进程，执行map, reduce任务  Hello, hi, hello -> map: hello,1 hi,1 hello,1 -> reduce: hello,2 hi,1
3. Master进程，管理任务的分配与状态监控
4. RPC连接: cppzmq rpc库
5. 超时重传：任务分配时启动计时器 -》 计时器线程等待超时 -》 任务完成前取消计时器 -》 任务超时重分配
