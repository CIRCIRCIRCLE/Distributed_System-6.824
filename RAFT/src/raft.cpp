#include "Raft.h"

void Raft::Make(vector<PeersInfo> peers, int id){
    m_peers = peers;
    //this->persister = persister;
    m_peerId = id;
    dead = 0;

    m_state = FOLLOWER;
    m_curTerm = 0;
    m_leaderId = -1;
    m_votedFor = -1;
    gettimeofday(&m_lastWakeTime, NULL);
    // readPersist(persister.ReadRaftState());

    // for(int i = 0; i < id + 1; i++){
    //     LogEntry log;
    //     log.m_command = to_string(i);
    //     log.m_term = i;
    //     m_logs.push_back(log);
    // }

    recvVotes = 0;
    finishedVote = 0;
    cur_peerId = 0;

    m_lastApplied = 0;
    m_commitIndex = 0;
    m_nextIndex.resize(peers.size(), 1);
    m_matchIndex.resize(peers.size(), 0);

    readRaftState();

    pthread_t listen_tid1;
    pthread_t listen_tid2;
    pthread_t listen_tid3;
    pthread_create(&listen_tid1, NULL, listenForVote, this);
    pthread_detach(listen_tid1);
    pthread_create(&listen_tid2, NULL, listenForAppend, this);
    pthread_detach(listen_tid2);
    pthread_create(&listen_tid3, NULL, applyLogLoop, this);
    pthread_detach(listen_tid3);
    /*
    pthread_create 函数用于创建线程：
    第一个参数是线程标识符。
    第二个参数设置线程属性。
    第三个参数是线程函数指针（例如 listenForVote）。
    第四个参数是传递给线程函数的参数（例如 this 指针，指向当前Raft对象）。    
    */
}

void* Raft::applyLogLoop(void* arg){
    Raft* raft = (Raft*)arg;
    while(!raft->dead){
        usleep(10000);
        raft->m_lock.lock();
        for(int i = raft->m_lastApplied; i < raft->m_commitIndex; i++){
            //遍历已提交但未应用的日志条目
            /**
             * @brief 封装好信息发回给客户端, LAB3中会用
             *     ApplyMsg msg;
             * 
             */
        }
        //更新 m_lastApplied：将 m_lastApplied 更新为 m_commitIndex，表示这些日志条目已经被应用到状态机。
        raft->m_lastApplied = raft->m_commitIndex;
        raft->m_lock.unlock();
    }
}

int Raft::getMyduration(timeval last){
    struct timeval now;
    gettimeofday(&now, NULL);
    // printf("--------------------------------\n");
    // printf("now's sec : %ld, now's usec : %ld\n", now.tv_sec, now.tv_usec);
    // printf("last's sec : %ld, last's usec : %ld\n", last.tv_sec, last.tv_usec);
    // printf("%d\n", ((now.tv_sec - last.tv_sec) * 1000000 + (now.tv_usec - last.tv_usec)));
    // printf("--------------------------------\n");
    return ((now.tv_sec - last.tv_sec) * 1000000 + (now.tv_usec - last.tv_usec));
}   

//-200000us是因为让记录的m_lastBroadcastTime变早，这样在appendLoop中getMyduration(m_lastBroadcastTime)直接达到要求
//因为心跳周期是100000us
void Raft::setBroadcastTime(){
    gettimeofday(&m_lastBroadcastTime, NULL);
    printf("before : %ld, %ld\n", m_lastBroadcastTime.tv_sec, m_lastBroadcastTime.tv_usec);
    if(m_lastBroadcastTime.tv_usec >= 200000){
        m_lastBroadcastTime.tv_usec -= 200000;
    }else{
        m_lastBroadcastTime.tv_sec -= 1;
        m_lastBroadcastTime.tv_usec += (1000000 - 200000);
    }
}

pair<int, bool> Raft::getState(){
    pair<int, bool> serverState;
    serverState.first = m_curTerm;
    serverState.second = (m_state == LEADER);
    return serverState;
}

bool Raft::checkLogUptodate(int term, int index){
    m_lock.lock();
    if(m_logs.size() == 0){
        m_lock.unlock();
        return true;
    }
    if(term > m_logs.back().m_term){
        m_lock.unlock();
        return true;
    }
    if(term == m_logs.back().m_term && index >= m_logs.size()){
        m_lock.unlock();
        return true;
    }
    m_lock.unlock();
    return false;
}

void Raft::push_backLog(LogEntry log){
    m_logs.push_back(log);
}

string Operation::getCmd(){
    string cmd = op + " " + key + " " + value;
    return cmd;
}

vector<LogEntry> Raft::getCmdAndTerm(string text){
    vector<LogEntry> logs;
    int n = text.size();
    vector<string> str;
    string tmp = "";
    for(int i = 0; i < n; i++){
        if(text[i] != ';'){
            tmp += text[i];
        }else{
            if(tmp.size() != 0) str.push_back(tmp);
            tmp = "";
        }
    }
    for(int i = 0; i < str.size(); i++){
        tmp = "";
        int j = 0;
        for(; j < str[i].size(); j++){
            if(str[i][j] != ','){
                tmp += str[i][j];
            }else break;
        }
        string number(str[i].begin() + j + 1, str[i].end());
        int num = atoi(number.c_str());
        logs.push_back(LogEntry(tmp, num));
    }
    return logs;
}

StartRet Raft::start(Operation op){
    StartRet ret;
    m_lock.lock();
    RAFT_STATE state = m_state;
    if(state != LEADER){
        m_lock.unlock();
        return ret;
    }
    ret.m_cmdIndex = m_logs.size();
    ret.m_curTerm = m_curTerm;
    ret.isLeader = true;

    LogEntry log;
    log.m_command = op.getCmd();
    log.m_term = m_curTerm;
    push_backLog(log);
    m_lock.unlock();
    
    return ret;
}

void Raft::printLogs(){
    for(auto a : m_logs){
        printf("logs : %d\n", a.m_term);
    }
    cout<<endl;
}

void Raft::kill(){
    dead = 1;
} 