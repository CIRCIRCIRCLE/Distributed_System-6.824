# Distributed_System-6.824

### Lab1: MapReduce
1. 编译程序
```cpp
cd mapreduce
1. 编译 map_reduceFun.cpp 并生成共享库libmrFunc.so
g++ -fpic -c map_reduceFun.cpp
g++ -shared map_reduceFun.o -o libmrFunc.so
2. 编译worker.cpp
g++ worker.cpp -ldl -o worker -I./buttonrpc-master -lzmq -pthread
3. 编译master.cpp
g++ master.cpp -o master -I./buttonrpc-master -lzmq -pthread
```
2. 运行
```cpp
LD_LIBRARY_PATH=. ./worker
set another terminal, run the following
./master  files/pg*.txt
```
