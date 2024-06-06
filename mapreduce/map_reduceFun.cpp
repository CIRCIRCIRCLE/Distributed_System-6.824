#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

class KeyValue {
public:
    string key;
    string value;
};

/*
Map 阶段将输入文本分割成单词并生成键值对，
Reduce 阶段对相同键的值进行汇总，
计算每个单词的总出现次数。

假设输入文本为 "Hello world Hello"
mapF: [{"Hello", "1"}, {"world", "1"}, {"Hello", "1"}]
reduceF: ["Hello 2", "world 1"]
*/

/**
 * split 函数：将输入文本按单词分割，返回一个单词列表。
 * @param text 传入的文本内容，数据量极大
 * @param length 传入的字符串长度
 * @return vector<string> 返回各个分割后的单词列表
 */
vector<string> split(char* text, int length) {
    vector<string> str;
    string tmp = "";
    for (int i = 0; i < length; i++) {
        if ((text[i] >= 'A' && text[i] <= 'Z') || (text[i] >= 'a' && text[i] <= 'z')) {
            tmp += text[i];
        } else {
            if (tmp.size() != 0) str.push_back(tmp);
            tmp = "";
        }
    }
    if (tmp.size() != 0) str.push_back(tmp);
    return str;
}

/**
 * mapF 函数：将输入文本按单词分割，并生成键值对，每个单词出现一次。
 * @brief mapFunc，需要打包成动态库，并在worker中通过dlopen以及dlsym运行时加载
 * @param kv 将文本按单词划分并以出现次数代表value长度存入keyValue
 * @return [{"Hello", "1"}, {"world", "1"}, {"Hello", "1"}]
 */
extern "C" vector<KeyValue> mapF(KeyValue kv) {
    vector<KeyValue> kvs;
    int len = kv.value.size();
    char content[len + 1];
    strcpy(content, kv.value.c_str());
    vector<string> str = split(content, len);
    for (const auto& s : str) {
        KeyValue tmp;
        tmp.key = s;
        tmp.value = "1";
        kvs.emplace_back(tmp);
    }
    return kvs;
}

/**
 * reduceF 函数：对输入的键值对进行汇总，计算每个键（单词）出现的总次数，并返回结果。
 * @brief reduceFunc，也是动态加载，输出对特定keyValue的reduce结果
 * @return ["Hello 2", "world 1"]
 */
extern "C" vector<string> reduceF(vector<KeyValue> kvs) {
    map<string, int> counts;
    for (const auto& kv : kvs) {
        counts[kv.key] += stoi(kv.value);
    }
    vector<string> str;
    for (const auto& count : counts) {
        str.push_back(count.first + " " + to_string(count.second));
    }
    return str;
}

