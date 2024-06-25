#include "Raft.h"

void Raft::saveRaftState(){
    persister.cur_term = m_curTerm;
    persister.votedFor = m_votedFor;
    persister.logs = m_logs;
    serialize();
}

void Raft::readRaftState(){
    //只在初始化的时候调用，没必要加锁，因为run()在其之后才执行
    bool ret = this->deserialize();
    if(!ret) return;
    this->m_curTerm = this->persister.cur_term;
    this->m_votedFor = this->persister.votedFor;

    for(const auto& log : this->persister.logs){
        push_backLog(log);
    }
    printf(" [%d]'s term : %d, votefor : %d, logs.size() : %d\n", m_peerId, m_curTerm, m_votedFor, m_logs.size());
}

void Raft::serialize(){
    // 构建持久化字符串
    string str;
    str += to_string(this->persister.cur_term) + ";" + to_string(this->persister.votedFor) + ";";
    for(const auto& log : this->persister.logs){
        str += log.m_command + "," + to_string(log.m_term) + ".";
    }

    // 打开文件
    string filename = "persister-" + to_string(m_peerId);
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0664);
    if(fd == -1){
        perror("open");
        exit(-1);
    }

    // 写入文件
    int len = write(fd, str.c_str(), str.size());
    if(len != str.size()){
        perror("write");
        exit(-1);
    }
    close(fd);
}

bool Raft::deserialize(){
    //文件名
    string filename = "persister-" + to_string(m_peerId);
    if(access(filename.c_str(), F_OK) == -1) return false;

    //打开文件
    int fd = open(filename.c_str(), O_RDONLY);
    if(fd == -1){
        perror("open");
        return false;
    }

    //读取文件内容
    int length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    char buf[length];
    bzero(buf, length);
    int len = read(fd, buf, length);
    if(len != length){
        perror("read");
        exit(-1);
    }
    close(fd);

    //解析文件内容
    string content(buf);
    vector<string> persist;
    string tmp = "";
    for(int i = 0; i < content.size(); i++){
        if(content[i] != ';'){
            tmp += content[i];
        }else{
            if(tmp.size() != 0) persist.push_back(tmp);
            tmp = "";
        }
    }
    persist.push_back(tmp);

    //恢复状态
    this->persister.cur_term = atoi(persist[0].c_str());
    this->persister.votedFor = atoi(persist[1].c_str());

    //恢复日志条目
    //1. 解析日志条目字符串
    vector<string> logEntries;   //字符串向量，存储解析出来的每个日志条目字符串
    vector<LogEntry> logs;
    tmp = "";  //临时字符串，用于累积日志条目的字符
    for(int i = 0; i < persist[2].size(); i++){
        if(persist[2][i] != '.'){
            tmp += persist[2][i];
        }else{
            if(tmp.size() != 0) logEntries.push_back(tmp);
            tmp = "";
        }
    }

    //2. 将字符串解析为 LogEntry 对象
    for(const auto& entry : logEntries){
        tmp = "";
        int j = 0;
        for(; j < entry.size(); j++){
            if(entry[j] != ','){
                tmp += entry[j];
            }else break;
        }
        string number(entry.begin() + j + 1, entry.end());
        int term = atoi(number.c_str());
        logs.push_back(LogEntry(tmp, term));
    }

    //3. 更新 persister 的日志条目
    this->persister.logs = logs;
    return true;
}