#include "Raft.h"

void* Raft::listenForVote(void* arg){
    Raft* raft = (Raft*)arg;
    buttonrpc server;
    server.as_server(raft->m_peers[raft->m_peerId].m_port.first);  //当前节点的投票端口
    server.bind("requestVote", &Raft::requestVote, raft);

    pthread_t wait_tid;
    pthread_create(&wait_tid, NULL, electionLoop, raft);
    pthread_detach(wait_tid);

    server.run();  //监听投票请求
    printf("exit!\n");
}

void* Raft::electionLoop(void* arg){
    Raft* raft = (Raft*)arg;
    bool resetFlag = false;
    while(!raft->dead){
        //设置200ms-400ms的随机超时时间
        int timeOut = rand()%200000 + 200000;
        while(1){
            usleep(1000); //内部循环每次暂停 1 毫秒（1000 微秒）。
            raft->m_lock.lock();

            int during_time = raft->getMyduration(raft->m_lastWakeTime);
            if(raft->m_state == FOLLOWER && during_time > timeOut){
                raft->m_state = CANDIDATE;
            }

            if(raft->m_state == CANDIDATE && during_time > timeOut){
                //节点成为候选人之后发起选举的全过程
                printf(" %d attempt election at term %d, timeOut is %d\n", raft->m_peerId, raft->m_curTerm, timeOut);
                gettimeofday(&raft->m_lastWakeTime, NULL);
                resetFlag = true;
                raft->m_curTerm++;
                raft->m_votedFor = raft->m_peerId;
                raft->saveRaftState();

                raft->recvVotes = 1;
                raft->finishedVote = 1;
                raft->cur_peerId = 0;
                
                //创建线程数组tid用于存储每个投票请求的线程ID
                //遍历其他所有节点：发送投票请求callRequestVote
                //分离线程
                pthread_t tid[raft->m_peers.size() - 1];
                int i = 0;
                for(auto server : raft->m_peers){
                    //跳过当前节点自己，避免向自己发送投票请求。
                    if(server.m_peerId == raft->m_peerId) continue;
                    pthread_create(tid + i, NULL, callRequestVote, raft);
                    pthread_detach(tid[i]);
                    i++;
                }

                while(raft->recvVotes <= raft->m_peers.size() / 2 && raft->finishedVote != raft->m_peers.size()){
                    //当收到的投票数超过半数或所有节点都完成投票时退出循环
                    raft->m_cond.wait(raft->m_lock.getLock());
                }
                if(raft->m_state != CANDIDATE){
                    //检查节点是否仍是候选人，不是则解锁并继续下一个循环
                    raft->m_lock.unlock();
                    continue;
                }
                if(raft->recvVotes > raft->m_peers.size() / 2){
                    raft->m_state = LEADER;

                    for(int i = 0; i < raft->m_peers.size(); i++){
                        raft->m_nextIndex[i] = raft->m_logs.size() + 1;
                        raft->m_matchIndex[i] = 0;
                    }

                    printf(" %d become new leader at term %d\n", raft->m_peerId, raft->m_curTerm);
                    raft->setBroadcastTime();
                }
            }
            raft->m_lock.unlock();
            if(resetFlag){
                resetFlag = false;
                break;
            }
        }   
    }
}

void* Raft::callRequestVote(void* arg){
    //发送投票请求并处理投票回复的线程函数
    Raft* raft = (Raft*) arg;
    buttonrpc client;
    raft->m_lock.lock();
    
    // 构造 RequestVoteArgs 参数
    RequestVoteArgs args;
    args.candidateId = raft->m_peerId;
    args.term = raft->m_curTerm;
    args.lastLogIndex = raft->m_logs.size();
    args.lastLogTerm = raft->m_logs.size() != 0 ? raft->m_logs.back().m_term : 0;

    //如果当前节点ID等于当前处理的节点ID，则增加当前处理的节点ID
    if(raft->cur_peerId == raft->m_peerId){
        raft->cur_peerId++;     
    }
    int clientPeerId = raft->cur_peerId;

    // 设置RPC客户端
    client.as_client("127.0.0.1", raft->m_peers[raft->cur_peerId++].m_port.first);

    // 检查并循环当前处理的节点ID
    if(raft->cur_peerId == raft->m_peers.size() || 
            (raft->cur_peerId == raft->m_peers.size() - 1 && raft->m_peerId == raft->cur_peerId)){
        raft->cur_peerId = 0;
    }
    raft->m_lock.unlock();

    // 发送投票请求并获取回复
    RequestVoteReply reply = client.call<RequestVoteReply>("requestVote", args).val();

    raft->m_lock.lock();
    raft->finishedVote++;
    raft->m_cond.signal(); 
    //raft->m_cond.signal()：通知其他等待的线程（通常是主线程），投票请求已经完成。这是通过条件变量 m_cond 实现的。

    if(reply.term > raft->m_curTerm){
        //当前节点的任期号已经过时，需要更新为跟随者状态，并将当前节点的任期号更新为回复中的任期号。
        raft->m_state = FOLLOWER;
        raft->m_curTerm = reply.term;
        raft->m_votedFor = -1;
        raft->readRaftState();
        raft->m_lock.unlock();
        return NULL;
    }

    //处理投票结果
    if(reply.VoteGranted){
        raft->recvVotes++;
    }
    raft->m_lock.unlock();
}


RequestVoteReply Raft::requestVote(RequestVoteArgs args){
    RequestVoteReply reply;
    reply.VoteGranted = false;
    m_lock.lock();
    reply.term = m_curTerm;

    if(m_curTerm > args.term){
        //请求任期小于当前任期，拒绝投票
        m_lock.unlock();
        return reply;
    }

    if(m_curTerm < args.term){
        //请求任期大于当前任期，更新当前任期并转换为跟随者:
        m_state = FOLLOWER;
        m_curTerm = args.term;
        m_votedFor = -1;
    }

    if(m_votedFor == -1 || m_votedFor == args.candidateId){
        m_lock.unlock();
        bool ret = checkLogUptodate(args.lastLogTerm, args.lastLogIndex);
        if(!ret) return reply;

        m_lock.lock();
        m_votedFor = args.candidateId;
        reply.VoteGranted = true;
        printf("[%d] vote to [%d] at %d, duration is %d\n", m_peerId, args.candidateId, m_curTerm, getMyduration(m_lastWakeTime));
        gettimeofday(&m_lastWakeTime, NULL);
    }
    saveRaftState();
    m_lock.unlock();
    return reply;
    /*
    1. 加锁和解锁: 由于投票操作可能涉及多个步骤（如日志检查），需要在一些耗时操作前解锁，操作完成后再加锁，确保不会因为长时间持有锁而导致死锁或性能问题。
    2. 检查日志是否最新: checkLogUptodate 方法用于确保候选人的日志至少和当前节点一样新，以保证日志的一致性。
    3. 状态更新: 当收到一个更高任期的投票请求时，当前节点需要更新自己的任期并转换为跟随者状态。
    4. 持久化状态: 每次状态更新后，都需要将状态持久化，以防止节点重启后丢失状态。
    */
}