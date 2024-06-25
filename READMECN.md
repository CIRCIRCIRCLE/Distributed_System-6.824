# Distributed_System-6.824
项目源代码来自： https://github.com/tjumcw/6.824/tree/main

### Lab1: MapReduce
#### Key Points:
1. Map和Reduce函数，动态加载并处理数据： Shuffle处理读取并聚合中间件（one worker process）, ReduceFunc聚合处理后的键值（all worker processes）
2. Worker进程，执行map, reduce任务  Hello, hi, hello -> map: hello,1 hi,1 hello,1 -> reduce: hello,2 hi,1
3. Master进程，管理任务的分配与状态监控
4. RPC连接: cppzmq rpc库
5. 超时重传：任务分配时启动计时器 -》 计时器线程等待超时 -》 任务完成前取消计时器 -》 任务超时重分配

### Lab2: RAFT
#### Key Points:
1. 领导者选举： 
    __listenForVote线程：__ 绑定requestVote，用于处理投票请求。    
    __创建electionLoop线程：__ 在选举期间实现选举逻辑，包括发起选举、请求投票和处理投票结果。     
2. 日志复制： 
    __listenForAppend线程：__ 绑定appendEntries，用于处理日志追加请求。   
    __创建processEntriesLoop线程：__ 在各节点循环进行日志复制，确保日志的一致性。    
3. 持久化：
    __读取持久化状态：__ 通过readRaft调用deserialize()从磁盘读取并加载状态。   
    __保存持久化状态：__ 通过saveRaft调用serialize()将当前状态序列化并写入磁盘。    
4. ApplyLogLoop:     
    __应用日志条目：__ 将已提交但未应用的日志条目应用于状态机，确保状态机与日志的一致性。    