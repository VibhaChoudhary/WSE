
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

using namespace std;
typedef std::pair<string, unsigned int> key;
typedef std::tuple<int,string,unsigned int,unsigned short int> posting;
namespace fs = std::filesystem;

struct compare_keys {
    bool operator()(key const & lhs, key const & rhs) const {
        if (lhs.first > rhs.first) return false;
        if (rhs.first > lhs.first) return true;
        return rhs.second > lhs.second;
    }
};

struct compare {
    bool operator()(const posting& lhs, const posting& rhs) const {
        if (get<1>(lhs).compare(get<1>(rhs)) > 0) return true;
        if (get<1>(lhs).compare(get<1>(rhs)) < 0) return false;
        return get<2>(lhs) > get<2>(rhs);
    }
};
class IndexBuilder {
public:
    IndexBuilder():postings_count{0},postings_file_num{1} {
        curr_term_id = 1;
        curr_byte_offset = 0;
    }
    map<key, unsigned short, struct compare_keys> postings;
    int merge_degree;
    int postings_buffer_size;
    int max_term_size;
    filesystem::path page_path;
    filesystem::path temp_path;
    filesystem::path data_path;
    unsigned int get_terms_count();
    unsigned int get_postings_count();
    unsigned int get_last_file_num();
    void create_postings(unsigned int,filesystem::path,int&);
    void merge_postings();
    void write_postings(map<key,unsigned short int,compare_keys>&,filesystem::path);
    void write_page_table();
    void write_term_id();
    void write_lexicon();

private:
    int curr_term_id;
    unsigned int curr_byte_offset;
    unsigned int postings_count;
    unsigned int postings_file_num;
    ofstream inv;
    unordered_map<string, int> term_id;
    map<int,pair<unsigned int,unsigned int>> lexicon;
    unordered_map<unsigned int,tuple<string,unsigned int,unsigned int>> page_table;
    vector<tuple<int,unsigned int,unsigned short int>> temp_postings;
    vector<char> separator = {' ',',',':','\\','/',';','|','.','-','"','(',')','_','<','>'};
    void vb_encode(unsigned int n,vector<unsigned char> &);
    void vb_decode(vector<unsigned char>,unsigned int &);
    void create_index(string,unsigned int, unsigned short int);
    void k_way_merge(filesystem::path, filesystem::path, int,int);
    void tokenize(unsigned int, string);
    bool is_valid_key(string &key);

    void write_temp_postings();
};
#endif //INDEXBUILDER_INDEXBUILDER_H
