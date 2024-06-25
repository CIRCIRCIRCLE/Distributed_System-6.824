#include "Raft.h"

void* Raft::listenForAppend(void* arg){
    Raft* raft = (Raft*)arg;
    buttonrpc server;
    server.as_server(raft->m_peers[raft->m_peerId].m_port.second); //日志追加端口
    server.bind("appendEntries", &Raft::appendEntries, raft);

    pthread_t heart_tid;
    pthread_create(&heart_tid, NULL, processEntriesLoop, raft);
    pthread_detach(heart_tid);

    server.run();
    printf("exit!\n");
}

void* Raft::processEntriesLoop(void* arg){
    Raft* raft = (Raft*)arg;
    while(!raft->dead){
        usleep(1000);
        raft->m_lock.lock();
        if(raft->m_state != LEADER){
            //检查状态：如果当前节点不是领导者，解锁并继续下一次循环。
            raft->m_lock.unlock();
            continue;
        }
        
        int during_time = raft->getMyduration(raft->m_lastBroadcastTime);
        if(during_time < HEART_BEART_PERIOD){
            //检查HeartBeat
            raft->m_lock.unlock();
            continue;
        }

        // 更新时间，解互斥锁允许其他线程访问共享数据
        gettimeofday(&raft->m_lastBroadcastTime, NULL);
        raft->m_lock.unlock();

        //创建并发送 AppendEntries 请求
        pthread_t tid[raft->m_peers.size() - 1];
        int i = 0;
        for(auto server : raft->m_peers){
            if(server.m_peerId == raft->m_peerId) continue; //跳过自身节点
            pthread_create(tid + i, NULL, sendAppendEntries, raft);
            pthread_detach(tid[i]);
            i++;
        }
    }
}

void* Raft::sendAppendEntries(void* arg){
    //1. 初始化和锁定
    Raft* raft = (Raft*)arg;
    buttonrpc client;
    AppendEntriesArgs args;
    raft->m_lock.lock();

    //2. 选择下一个跟随者节点,设置RPC客户端
    if(raft->cur_peerId == raft->m_peerId){//跳过自己
        raft->cur_peerId++;     
    }
    int clientPeerId = raft->cur_peerId;
    client.as_client("127.0.0.1", raft->m_peers[raft->cur_peerId++].m_port.second);

    //3. 构造 AppendEntries 请求参数
    args.m_term = raft->m_curTerm;
    args.m_leaderId = raft->m_peerId;
    args.m_prevLogIndex = raft->m_nextIndex[clientPeerId] - 1;
    args.m_leaderCommit = raft->m_commitIndex;    
    //raft->m_nextIndex[clientPeerId] 是领导者认为跟随者节点的下一个日志条目的索引
    //因此前一个日志条目的索引是 raft->m_nextIndex[clientPeerId] - 1。


    for(int i = args.m_prevLogIndex; i < raft->m_logs.size(); i++){
        //遍历从 args.m_prevLogIndex 开始的所有日志条目，将它们的命令和任期号构造成字符串。
        //例如，如果日志条目包含命令 set x=1 和任期 2，则构造成 "set x=1,2;"。
        args.m_sendLogs += (raft->m_logs[i].m_command + "," + to_string(raft->m_logs[i].m_term) + ";");
    }

    //设置前一个日志条目的任期
    if(args.m_prevLogIndex == 0){
        //这是第一个日志条目
        args.m_prevLogTerm = 0;
        if(raft->m_logs.size() != 0){
            args.m_prevLogTerm = raft->m_logs[0].m_term;
        }
    }
    else args.m_prevLogTerm = raft->m_logs[args.m_prevLogIndex - 1].m_term;

    printf("[%d] -> [%d]'s prevLogIndex : %d, prevLogTerm : %d\n", raft->m_peerId, clientPeerId, args.m_prevLogIndex, args.m_prevLogTerm);

    //4. 处理 cur_peerId 溢出    
    if(raft->cur_peerId == raft->m_peers.size() || 
            (raft->cur_peerId == raft->m_peers.size() - 1 && raft->m_peerId == raft->cur_peerId)){
        raft->cur_peerId = 0;
    }
    raft->m_lock.unlock();

    //5. 发送 AppendEntries 请求
    AppendEntriesReply reply = client.call<AppendEntriesReply>("appendEntries", args).val();

    //6. 处理回复
    raft->m_lock.lock();
    if(reply.m_term > raft->m_curTerm){
        //处理任期冲突：如果回复中的任期大于当前节点的任期，则转换为跟随者，并更新任期
        raft->m_state = FOLLOWER;
        raft->m_curTerm = reply.m_term;
        raft->m_votedFor = -1;
        raft->saveRaftState();
        raft->m_lock.unlock();
        return NULL;                        //FOLLOWER没必要维护nextIndex,成为leader会更新
    }

    // 7. 处理成功的回复
    if(reply.m_success){
        //更新日志索引：更新 m_nextIndex 和 m_matchIndex。
        raft->m_nextIndex[clientPeerId] += raft->getCmdAndTerm(args.m_sendLogs).size();
        raft->m_matchIndex[clientPeerId] = raft->m_nextIndex[clientPeerId] - 1;   


        //更新提交索引：根据大多数节点的匹配索引，更新提交索引。
        vector<int> tmpIndex = raft->m_matchIndex;
        sort(tmpIndex.begin(), tmpIndex.end()); //将 m_matchIndex 复制到临时向量 tmpIndex 并排序，以便找出大多数节点的匹配索引。 
        int realMajorityMatchIndex = tmpIndex[tmpIndex.size() / 2]; //由于 tmpIndex 已排序，中间位置的索引即为大多数节点的匹配索引
        if(realMajorityMatchIndex > raft->m_commitIndex && raft->m_logs[realMajorityMatchIndex - 1].m_term == raft->m_curTerm){
            //确保该日志条目在当前任期内被复制（保证领导者的正确性）。
            raft->m_commitIndex = realMajorityMatchIndex;
        }
    }

    if(!reply.m_success){
        // if(!raft->m_firstIndexOfEachTerm.count(reply.m_conflict_term)){
        //     raft->m_nextIndex[clientPeerId]--;
        // }else{
        //     raft->m_nextIndex[clientPeerId] = min(reply.m_conflict_index, raft->m_firstIndexOfEachTerm[reply.m_conflict_term]);
        // }   

        if(reply.m_conflict_term != -1){
            int leader_conflict_index = -1;
            for(int index = args.m_prevLogIndex; index >= 1; index--){
                if(raft->m_logs[index - 1].m_term == reply.m_conflict_term){
                    leader_conflict_index = index;
                    break;
                }
            }
            if(leader_conflict_index != -1){
                raft->m_nextIndex[clientPeerId] = leader_conflict_index + 1;
            }else{
                raft->m_nextIndex[clientPeerId] = reply.m_conflict_index;
            }
        }else{
            raft->m_nextIndex[clientPeerId] = reply.m_conflict_index + 1;
        }
        
    }
    raft->saveRaftState();
    raft->m_lock.unlock();

}

AppendEntriesReply Raft::appendEntries(AppendEntriesArgs args){
    //从接收到的 args.m_sendLogs 中解码日志条目。
    //1.初始化回复结构 reply，设置默认值。
    vector<LogEntry> recvLog = getCmdAndTerm(args.m_sendLogs);
    AppendEntriesReply reply;
    m_lock.lock();
    reply.m_term = m_curTerm;
    reply.m_success = false;
    reply.m_conflict_index = -1;
    reply.m_conflict_term = -1;


    //2. 检查领导者的任期
    if(args.m_term < m_curTerm){
        //如果领导者的任期小于当前节点的任期，直接返回失败的回复
        m_lock.unlock();
        return reply;
    }

    if(args.m_term >= m_curTerm){
        //如果领导者的任期大于或等于当前节点的任期，更新当前节点的任期，并将其状态设置为跟随者。
        if(args.m_term > m_curTerm){
            m_votedFor = -1;
            saveRaftState();
        }
        m_curTerm = args.m_term;
        m_state = FOLLOWER;

    }
    printf("[%d] recv append from [%d] at self term%d, send term %d, duration is %d\n",
            m_peerId, args.m_leaderId, m_curTerm, args.m_term, getMyduration(m_lastWakeTime));
    gettimeofday(&m_lastWakeTime, NULL);
    // persister()

    //3. 日志为空时的处理
    int logSize = 0;
    if(m_logs.size() == 0){
        for(const auto& log : recvLog){
            push_backLog(log);
        }
        saveRaftState();
        logSize = m_logs.size();
        if(m_commitIndex < args.m_leaderCommit){
            //跟随者节点需要确保它的已提交日志索引不超过领导者的已提交日志索引
            m_commitIndex = min(args.m_leaderCommit, logSize);
        }
        // persister.persist_lock.lock();
        // persister.cur_term = m_curTerm;
        // persister.votedFor = m_votedFor;
        // persister.logs = m_logs;
        // persister.persist_lock.unlock();
        m_lock.unlock();
        reply.m_success = true;
        // saveRaftState();
        return reply;
    }

    //4.检查前一个日志条目是否匹配
    if(m_logs.size() < args.m_prevLogIndex){
        //检查接收到的日志条目之前的日志是否存在并且匹配。
        printf(" [%d]'s logs.size : %d < [%d]'s prevLogIdx : %d\n", m_peerId, m_logs.size(), args.m_leaderId, args.m_prevLogIndex);
        //设置 reply.m_conflict_index 为当前节点日志的大小（即冲突发生的位置）。
        reply.m_conflict_index = m_logs.size();    //索引要加1
        m_lock.unlock();
        reply.m_success = false;
        return reply;
    }
    if(args.m_prevLogIndex > 0 && m_logs[args.m_prevLogIndex - 1].m_term != args.m_prevLogTerm){
        //follower节点的前一个日志条目的任期是否与leader提供的任期匹配。
        //如果日志条目之前的日志不存在或不匹配，返回冲突的索引和任期。
        printf(" [%d]'s prevLogterm : %d != [%d]'s prevLogTerm : %d\n", m_peerId, m_logs[args.m_prevLogIndex - 1].m_term, args.m_leaderId, args.m_prevLogTerm);

        reply.m_conflict_term = m_logs[args.m_prevLogIndex - 1].m_term;
        for(int index = 1; index <= args.m_prevLogIndex; index++){
            if(m_logs[index - 1].m_term == reply.m_conflict_term){
                reply.m_conflict_index = index;                         //找到冲突term的第一个index,比索引要加1
                break;
            }
        }
        m_lock.unlock();
        reply.m_success = false;
        return reply;
    }

    //5. 删除冲突日志条目并追加新日志条目
    logSize = m_logs.size();
    for(int i = args.m_prevLogIndex; i < logSize; i++){
        m_logs.pop_back();
    }
    // m_logs.insert(m_logs.end(), recvLog.begin(), recvLog.end());
    for(const auto& log : recvLog){
        push_backLog(log);
    }
    saveRaftState();
    logSize = m_logs.size();
    if(m_commitIndex < args.m_leaderCommit){
        m_commitIndex = min(args.m_leaderCommit, logSize);
    }
    for(auto a : m_logs) printf("%d ", a.m_term);
    printf(" [%d] sync success\n", m_peerId);
    m_lock.unlock();
    reply.m_success = true;
    return reply;
}