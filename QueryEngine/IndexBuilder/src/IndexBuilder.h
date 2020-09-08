
#ifndef INDEXBUILDER_INDEXBUILDER_H
#define INDEXBUILDER_INDEXBUILDER_H
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
#include <unordered_set>


using namespace std;
typedef std::pair<unsigned long long int, unsigned int> key;

typedef std::tuple<int,string,unsigned int,unsigned short int> posting;
typedef std::tuple<int,unsigned long long int,unsigned int,unsigned short int> posting2;

namespace fs = std::filesystem;



struct compare_keys {
    bool operator()(key const & lhs, key const & rhs) const {
        if (lhs.first > rhs.first) return false;
        if (rhs.first > lhs.first) return true;
        return rhs.second > lhs.second;
    }
};

struct compare {
    bool operator()(const posting2& lhs, const posting2& rhs) const {
        if (get<1>(lhs) > get<1>(rhs)) return true;
        if (get<1>(lhs) < get<1>(rhs)) return false;
        return get<2>(lhs) > get<2>(rhs);
    }
};

class IndexBuilder {
public:
    IndexBuilder():postings_count{0},postings_file_num{1} {
        curr_term_id = 1;
        curr_byte_offset = 0;
        docid = 0;
    }
    map<key, unsigned short, struct compare_keys> postings;

    int merge_degree;
    int postings_buffer_size;
    int max_term_size;
    int docid;
    filesystem::path page_path;
    filesystem::path temp_path;
    filesystem::path data_path;
    unsigned long long int get_terms_count();
    unsigned long long int get_postings_count();
    unsigned long int get_last_file_num();
    void create_postings(filesystem::path);
    void merge_postings();
    void write_map_postings(map<key, unsigned short, struct compare_keys>&,filesystem::path);
    void write_page_table();
    void write_term_id();
    void write_lexicon();

    void create_files(filesystem::path p);

private:
    unsigned long long int curr_term_id;
    unsigned long long int curr_byte_offset;
    unsigned long long int postings_count;
    unsigned long int postings_file_num;
    ofstream inv;
    ofstream lex;
    unordered_map<string, unsigned long long int> term_id;
    unordered_map<unsigned int,pair<unsigned long long int,unsigned int>> lexicon;
    unordered_map<unsigned int,tuple<string,string,unsigned int,unsigned int>> page_table;
    vector<tuple<unsigned long long int,unsigned int,unsigned short int>> temp_postings;
    vector<char> separator = {' ',',','\\','/',';','|','.','-','_','(',')','<','>','\n','\r'};
    void create_index(unsigned long long int,unsigned int, unsigned short int);
    void k_way_merge(filesystem::path, filesystem::path, int,int);
    void tokenize(unsigned int, string);
    bool is_valid_key(string &key);
    void write_index();
    void vb_encode(unsigned int n, vector<unsigned char> &result);

};
#endif //INDEXBUILDER_INDEXBUILDER_H
