## Lab2: RAFT
### 目录结构：
```cpp
Raft/
├── buttonrpc/
│   ├── buttonrpc.hpp
│   ├── buttonrpc.cpp
│   └── buttonrpc_impl.hpp
├── include/
│   ├── Raft.h
│   └── locker.h
├── src/
│   ├── LeaderElection.cpp
│   ├── LogEntriesAppend.cpp
│   ├── StatePersistence.cpp
│   ├── Raft.cpp
│   └── main.cpp

```
### 程序编译与运行:
```cpp
编译命令：g++ -o raft src/*.cpp buttonrpc/buttonrpc.cpp -Iinclude -Ibuttonrpc -lzmq -pthread   
生成可执行文件： ./raft 3（节点数）
```
### 文件功能：
#### Leader Election (领导者选举)   
listenForVote：监听投票请求的线程函数。  
electionLoop：持续进行领导者选举的循环。  
callRequestVote：发送投票请求的线程函数。  
requestVote：处理收到的投票请求并进行投票。  

#### Log Entries Append (日志追加)   
listenForAppend：监听日志追加请求的线程函数。  
processEntriesLoop：处理日志同步的循环。  
sendAppendEntries：发送日志追加请求的线程函数。  
appendEntries：处理收到的日志追加请求并进行日志追加。  
State Persistence (状态持久化)  

#### State Persistence (状态持久化)
saveRaftState：保存当前Raft状态到持久化存储。  
readRaftState：从持久化存储读取Raft状态。  
serialize：将当前状态序列化为字符串并写入文件。  
deserialize：从文件中读取字符串并反序列化为当前状态。  

####  辅助功能
Operation：操作类，定义了Raft日志中的操作。  
StartRet：启动返回类，封装了日志追加的返回结果。  
ApplyMsg：应用消息类，定义了应用到上层应用的日志条目。  
PeersInfo：存储节点信息的类，包括节点ID和两个RPC端口。  
LogEntry：日志条目类，包含命令和任期号。  
Persister：持久化类，存储当前日志和状态。  
AppendEntriesArgs：日志追加请求参数类。  
AppendEntriesReply：日志追加请求回复类。  
RequestVoteArgs：选票请求参数类。  
RequestVoteReply：选票请求回复类。  

#### 遇到的问题    
1. 多重定义问题： 在多个编译单元中重复定义模板函数导致链接错误。  
解决方案：将模板函数的实现从头文件分离出来，放到单独的 buttonrpc_impl.hpp 文件中，并在 buttonrpc.hpp 中包含它。  

2. 线程函数指针问题：使用成员函数作为线程函数指针时类型不匹配。     
解决方案：将线程函数声明为静态成员函数，确保它们符合 pthread_create 的参数要求。  

3. 并发访问问题： 多线程访问共享数据时出现竞争条件。  
解决方案：使用互斥锁和条件变量保护共享数据，确保线程安全。  

#### 总结
本项目实现了Raft分布式一致性算法的核心功能，包括领导者选举、日志追加和状态持久化。通过使用轻量级RPC库 buttonrpc 实现节点之间的通信，并使用多线程和同步机制保证并发安全。项目在开发过程中遇到了一些技术难题，通过模块化设计和逐步调试成功解决。最终，通过整合和测试验证了实现的正确性和可靠性。   