#ifndef QUERYPROCESSOR_QUERYPROCESSOR_H
#define QUERYPROCESSOR_QUERYPROCESSOR_H
#include <iostream>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <map>
#include <queue>
#include <tuple>
#include <algorithm>
#include <string_view>
#include <string>
#include <cmath>
using namespace std;
namespace fs = std::filesystem;
//data structure for storing term level information, inverted index length and term frequencies
typedef vector<tuple<string,unsigned int,unsigned int>> term_information;
//data structure for storing query return results such as url,location,score,etc.
typedef vector<tuple<string,string,string,double,unsigned int,double,term_information>> query_result;
//data structure for storing term information at the time of query processing
typedef map<pair<unsigned int,int>, pair<unsigned int,unsigned int>> information2;

struct List{
    int doc_count;
    vector<unsigned int> postings;
    int iterator;
    unsigned int base;
};
struct compare_doc{
    bool operator()(const pair<unsigned int,double> &lhs,const pair<unsigned int,double> &rhs){
        if(lhs.second > rhs.second) return true;
        else return false;
    }
};
struct compare_line{
    bool operator()(const pair<int,int> &lhs,const pair<int,int> &rhs){
        if(lhs.second < rhs.second) return true;
        else return false;
    }
};

typedef priority_queue<pair<unsigned int,double>,vector<pair<unsigned int,double>>,compare_doc> pq_top;

class QueryProcessor{
public:
    QueryProcessor() {
        BM_k1 = 0.75;
        BM_b = 1.2;

    }
    filesystem::path lexicon;
    filesystem::path page_table;
    filesystem::path term_mapping;
    filesystem::path inverted_index;
    void init();
    query_result processQuery(string,bool conj=1);



private:
    //define constants used for BM25
    unsigned int BM_N;
    double BM_avg;
    double BM_k1;
    double BM_b;
    unsigned int maxId;

    //define data structures to store data structures in memory
    unordered_map<string, unsigned long int> term_map;
    unordered_map<unsigned int,tuple<string,string,unsigned int,unsigned int>> ptable;
    unordered_map<unsigned int,pair<unsigned long long int,unsigned int>> lex;

    //used for tokenize query
    vector<char> separator = {' ',',','\\','/',';','|','.','-','_','(',')','<','>','\n','\r'};
    void tokenize_query(string,vector<string>&);


    void read_page_table();
    void read_lexicon();
    void read_term_mapping();

    void processConjunctiveQuery( vector<List*>&,int,pq_top&,information2&);
    void processDisjunctiveQuery( vector<List*>&,int,pq_top&,information2&);


    List * openList(string term);
    unsigned int nextGEQ(List*&, unsigned int);
    unsigned int getFreq(List*&, unsigned int);
    void closeList(List *&lp);

    void vb_decode(vector<unsigned char>,vector<unsigned int> &);

    string get_snippet(unsigned int,const vector<string>&);


};
#endif //QUERYPROCESSOR_QUERYPROCESSOR_H
