#ifndef RAFT_H
#define RAFT_H

#include <vector>
#include <string>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include "locker.h"
#include "../buttonrpc/buttonrpc.hpp"

#define COMMOM_PORT 1234
#define HEART_BEART_PERIOD 100000

class Operation {
public:
    std::string getCmd();
    std::string op;
    std::string key;
    std::string value;
    int clientId;
    int requestId;
};

class StartRet {
public:
    StartRet():m_cmdIndex(-1), m_curTerm(-1), isLeader(false) {}
    int m_cmdIndex;
    int m_curTerm;
    bool isLeader;
};

class ApplyMsg {
public:
    bool CommandValid;
    std::string command;
    int CommandIndex;
};

//store current Raft ID and two port numbers(one for election, the other for log entries)
class PeersInfo {
public:
    std::pair<int, int> m_port;
    int m_peerId;
};

class LogEntry {
public:
    LogEntry(std::string cmd = "", int term = -1): m_command(cmd), m_term(term) {}
    std::string m_command;
    int m_term;
};

class Persister {
public:
    std::vector<LogEntry> logs;
    int cur_term;
    int votedFor;
};

class AppendEntriesArgs {
public:
    int m_term;
    int m_leaderId;
    int m_prevLogIndex; 
    int m_prevLogTerm;  //the two params are used for Consistency Check
    int m_leaderCommit;
    std::string m_sendLogs;
    friend Serializer& operator >> (Serializer& in, AppendEntriesArgs& d) {
		in >> d.m_term >> d.m_leaderId >> d.m_prevLogIndex >> d.m_prevLogTerm >> d.m_leaderCommit >> d.m_sendLogs;
		return in;
	}
	friend Serializer& operator << (Serializer& out, AppendEntriesArgs d) {
		out << d.m_term << d.m_leaderId << d.m_prevLogIndex << d.m_prevLogTerm << d.m_leaderCommit << d.m_sendLogs;
		return out;
	}
};

class AppendEntriesReply {
public:
    int m_term;
    bool m_success;
    int m_conflict_term;
    int m_conflict_index;  //Used for fast log matching in case of conflicts
};

class RequestVoteArgs {
public:
    int term;
    int candidateId;
    int lastLogTerm;
    int lastLogIndex;
};

class RequestVoteReply {
public:
    int term;
    bool VoteGranted;
};

class Raft {
public:
    enum RAFT_STATE { LEADER = 0, CANDIDATE, FOLLOWER };

    //Leader Election
    static void* listenForVote(void* arg);
    static void* electionLoop(void* arg);
    static void* callRequestVote(void* arg);
    RequestVoteReply requestVote(RequestVoteArgs args);

    //Log Entries Append
    static void* listenForAppend(void* arg);
    static void* processEntriesLoop(void* arg);
    static void* sendAppendEntries(void* arg);
    AppendEntriesReply appendEntries(AppendEntriesArgs args);

    //State Persistence
    void saveRaftState();
    void readRaftState();
    void serialize();
    bool deserialize();

    void Make(std::vector<PeersInfo> peers, int id);
    static void* applyLogLoop(void* arg); 
    int getMyduration(timeval last);
    void setBroadcastTime();
    std::pair<int, bool> getState();
    bool checkLogUptodate(int term, int index);
    void push_backLog(LogEntry log);
    std::vector<LogEntry> getCmdAndTerm(std::string text);
    StartRet start(Operation op);
    void printLogs();
    bool isKilled();
    void kill();  

    locker m_lock;
    cond m_cond;
    std::vector<PeersInfo> m_peers;

    int m_peerId;
    int dead;

    //Data that needs to be persisted
    Persister persister;  
    int m_curTerm;
    int m_votedFor;
    std::vector<LogEntry> m_logs;

    std::vector<int> m_nextIndex;
    std::vector<int> m_matchIndex;
    int m_lastApplied;
    int m_commitIndex;

    int recvVotes;
    int finishedVote;
    int cur_peerId;

    RAFT_STATE m_state;
    int m_leaderId;
    struct timeval m_lastWakeTime;
    struct timeval m_lastBroadcastTime;
};

#endif // RAFT_H
