#include "Raft.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("loss parameter of peersNum\n");
        exit(-1);
    }
    int peersNum = std::atoi(argv[1]);
    if (peersNum % 2 == 0) {
        printf("the peersNum should be odd\n");
        exit(-1);
    }
    srand((unsigned)time(NULL));
    std::vector<PeersInfo> peers(peersNum);
    for (int i = 0; i < peersNum; i++) {
        peers[i].m_peerId = i;
        peers[i].m_port.first = COMMOM_PORT + i;
        peers[i].m_port.second = COMMOM_PORT + i + peers.size();
    }

    Raft* raft = new Raft[peers.size()];
    for (int i = 0; i < peers.size(); i++) {
        raft[i].Make(peers, i);
    }

    // Test section
    usleep(400000);
    for (int i = 0; i < peers.size(); i++) {
        if (raft[i].getState().second) {
            for (int j = 0; j < 1000; j++) {
                Operation opera;
                opera.op = "put";
                opera.key = std::to_string(j);
                opera.value = std::to_string(j);
                raft[i].start(opera);
                usleep(50000);
            }
        } else continue;
    }
    usleep(400000);
    for (int i = 0; i < peers.size(); i++) {
        if (raft[i].getState().second) {
            raft[i].kill();
            break;
        }
    }

    while (1);
}
