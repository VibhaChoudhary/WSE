#include "QueryProcessor.h"

string ModifyPath(string input) {
    replace(input.begin(), input.end(), '\\', '/');
    input.replace(input.begin(), input.begin() + 2, "/indexer");
    return input;
}

void QueryProcessor::init() {
    cout<<"reading lexicon"<<endl;
    read_lexicon();
    cout<<"reading page_table"<<endl;
    read_page_table();
    cout<<"reading term mapping"<<endl;
    read_term_mapping();
}
void QueryProcessor::read_page_table() {
    ifstream ifs(page_table,ios::in);
    unsigned int docid;
    string url;
    string path;
    unsigned int doc_length;
    unsigned int doc_id_max = 0;
    unsigned int page_rank;
    double doc_length_sum = 0.0;
    string line;
    while(getline(ifs,line)){
        int pos1 = line.find_first_of(" ");
        int pos2 = line.find_first_of(" ",pos1+1);
        int pos3 = line.find_first_of(" ",pos2+1);
        int pos4 = line.find_first_of(" ",pos3+1);
        docid = stoi(line.substr(0,pos1));
        doc_id_max = max(doc_id_max,docid);
        url = line.substr(pos1+1,pos2-1-pos1);
        path = line.substr(pos2+1,pos3-1-pos2);
        doc_length = stoi(line.substr(pos3+1,pos4-1-pos3));
        doc_length_sum += doc_length;
        page_rank = stoi(line.substr(pos4+1));
        ptable[docid] = make_tuple(url,path,doc_length,page_rank);
        //cout<<docid<<" "<<url<<" "<<path<<" "<<doc_length<<" "<<page_rank<<endl;
        if(ifs.eof()) break;
    }
    BM_N = doc_id_max;
    maxId = BM_N + 1;
    BM_avg = doc_length_sum/BM_N;
    ifs.close();
}
void QueryProcessor::read_lexicon() {
    ifstream ifs(lexicon,ios::in);
    string l;
    unsigned int tid;
    unsigned long long int start_byte;
    unsigned int num_bytes;
    while(getline(ifs,l)){
        string line = l;
        int pos1 = line.find_first_of(" ");
        int pos2 = line.find_first_of(" ",pos1+1);
        tid = stoi(line.substr(0,pos1));
        start_byte = stoll(line.substr(pos1+1,pos2-1-pos1));
        num_bytes = stoi(line.substr(pos2+1));
        lex[tid] = make_pair(start_byte,num_bytes);
        //cout<<tid<<" "<<start_byte<<" "<<num_bytes<<endl;
        if(ifs.eof()) break;
    }
    ifs.close();
}
void QueryProcessor::read_term_mapping() {
    ifstream ifs(term_mapping,ios::in);
    unsigned long int tid;
    string term;
    string line;
    while(getline(ifs,line)){
        int pos1 = line.find_first_of(" ");
        tid = stol(line.substr(pos1));
        term = line.substr(0,pos1);
        term_map[term] = tid;
        //cout<<term<<" "<<tid<<endl;
        if(ifs.eof()) break;
    }
    ifs.close();
}

query_result QueryProcessor::processQuery(string query,bool if_conjuctive) {
    query_result res;
    pq_top top_10;
    information2 info;
    //collect all the terms in the query
    vector<string> terms;
    tokenize_query(query,terms);
    int num_terms = terms.size();

    //Create list pointers for each term inverted list and get shortest list
    vector<List *> lp(num_terms,nullptr);
    int min_index = 0;
    int minimum = maxId;
    for(int i=0;i<num_terms; i++){
        lp[i] = openList(terms[i]);
        if(lp[i]!=nullptr){
            int new_size = lp[i]->doc_count;
            if(new_size < minimum) {
                minimum = new_size;
                min_index = i;
            }
        }
        else{
            if(if_conjuctive == 1)
                return res;
        }
    }
    //no results found
    if(minimum == maxId) return res;
    //make shortest list the first list;
    swap(lp[0],lp[min_index]);
    //process query
    if(if_conjuctive){
        processConjunctiveQuery(lp,num_terms,top_10,info);
    }
    else{
        processDisjunctiveQuery(lp,num_terms,top_10,info);
    }
    while(!top_10.empty()){
        auto ele = top_10.top();
        top_10.pop();
        string snippet = get_snippet(ele.first,terms);
        string url = get<0>(ptable[ele.first]);
        string path = get<1>(ptable[ele.first]);
        double score = ele.second;
        unsigned int doc_length = get<2>(ptable[ele.first]);
        if(doc_length==0)
            continue;
        path = ModifyPath(path);
        term_information term_info;
        for(int i=0;i<num_terms;i++) {
            if (info.find(make_pair(ele.first, i)) != info.end()) {
                auto freq = info[make_pair(ele.first, i)];
                term_info.push_back(make_tuple(terms[i], freq.first, freq.second));
            }
        }
        res.push_back(make_tuple(url,path,snippet,score,doc_length,BM_avg,term_info));

    }
    return res;
}
void QueryProcessor::vb_decode(vector<unsigned char> input,vector<unsigned int> &numbers){
    unsigned int n=0;
    int i=0;
    int d=1;
    for(int i=0;i<input.size();i++){
        if(input[i] < 128){
           // n = 128*n + input[i];
           n = n + d * input[i];
           d = d * 128;
        }
        else{
            //n = 128*n + (input[i]-128);
            n = n + d * (input[i] - 128);
            numbers.push_back(n);
            n=0;
            d=1;
        }
    }
    if(n!=0) numbers.push_back(n);
}

void QueryProcessor::tokenize_query(string query, vector<string> &terms) {
    string key="";
    //parse line
    for(char c : query){
        //if any separator from the list is found save the current buffered word
        if(find(separator.begin(),separator.end(),c) == separator.end()){
            if(!ispunct(c) && isprint(c) && isalnum(c)){
                c = tolower(c);
                key += c;
            }
        }
        else{
          terms.push_back(key);
          key="";
        }

    }
    if(!key.empty()) terms.push_back(key);

}
List * QueryProcessor::openList(string term) {
    if(term_map.find(term) == term_map.end()){
        cout<<"term "<<term<<" not found"<<endl;
        return nullptr;
    }
    ifstream ifs(inverted_index,ios::binary);
    unsigned long long int start = lex[term_map[term]].first;
    unsigned int end = lex[term_map[term]].second;
    ifs.seekg(start);
    List *lp = new List();
    unsigned char buffer[end];
    ifs.read(reinterpret_cast<char *>(buffer), end);
    vector<unsigned char> postings;
    postings.assign(buffer,buffer+end);
    vector<unsigned int> posts;
    vb_decode(postings, lp->postings);
    lp->doc_count = lp->postings.size()/2;
    lp->iterator = 0;
    lp->base = 0;
    ifs.close();
    return lp;
}
void QueryProcessor::closeList(List *&lp) {
    ;
}
unsigned int QueryProcessor::getFreq(List *&lp, unsigned int did) {
    return lp->postings[lp->iterator - 1];
}
unsigned int QueryProcessor::nextGEQ(List *&lp, unsigned int did) {
    int start = lp->iterator;
    int result = 0;
    while(start < 2*lp->doc_count){
        if((lp->postings[start] + lp->base) >= did) {
            result = lp->postings[start] + lp->base;
            break;
        }
        lp->base = lp->postings[start] + lp->base;
        start += 2;
    }
    if(result == 0)
        return maxId;
    if(start < 2*lp->doc_count){
        lp->iterator = start+2;
        lp->base = lp->postings[start] + lp->base;
    }
    return result;
}

string QueryProcessor::get_snippet(unsigned int did, const vector<string> &terms) {
    string result = "";
    string path = get<1>(ptable[did]);
    path = ModifyPath(path);
    string line;
    ifstream ifs(path, ifstream::in);

    unordered_map<int, pair<string, vector<tuple<string, int, int>>>> lines;
    priority_queue<pair<int, int>, vector<pair<int, int>>, compare_line> pq;
    if (ifs) {
        for (int i = 0; i < 7; i++)
            getline(ifs, line);
        int i = 1;
        while (getline(ifs, line)) {
            string key = "";
            int curPos = 0;
            int termStartPos = 0;
            for (char c :line) {
                if (find(separator.begin(), separator.end(), c) == separator.end()) {
                    if (!ispunct(c) && isprint(c) && isalnum(c)) {
                        c = tolower(c);
                        key += c;
                    }
                } else {
                    for (string term :terms) {
                        if (key == term) {
                            if (lines.find(i) != lines.end())
                                lines[i].second.push_back(make_tuple(key, termStartPos, curPos - 1));
                            else {
                                vector<tuple<string, int, int>> highlights;
                                highlights.push_back(make_tuple(key, termStartPos, curPos - 1));
                                lines[i] = make_pair(line, highlights);
                            }
                            break;
                        }
                    }
                    key = "";
                    termStartPos = curPos + 1;
                }
                curPos++;
            }
            if (ifs.eof())
                break;
            i++;
        }
    }

    int frag_no = 1;
    for (auto ele :lines)
        pq.emplace(make_pair(ele.first, ele.second.second.size()));
    //create snippet with top 4 lines
    while (frag_no <= 4 && !pq.empty()) {
        auto ele = pq.top();
        pq.pop();

        line = lines[ele.first].first;
        vector<tuple<string, int, int>> highlights = lines[ele.first].second;
        string frag = "";
        if (frag_no > 1) {
            frag = "...";
        }

        int prevEndPos = 0;
        for (int i = 0; i < highlights.size(); i++) {
            string highlight = get<0>(highlights[i]);
            int termStartPos = get<1>(highlights[i]);
            int termEndPos = get<2>(highlights[i]);

            if (termStartPos > 0) {
                if (termStartPos - prevEndPos <= 50) {
                    frag += line.substr(prevEndPos, termStartPos - prevEndPos);
                } else {
                    if (prevEndPos == 0) {
                        frag += line.substr(termStartPos - 50, 50);
                    } else {
                        if (termStartPos - prevEndPos <= 100) {
                            frag += line.substr(prevEndPos, termStartPos - prevEndPos);
                        } else {
                            frag += line.substr(prevEndPos, 50) + "..." + line.substr(termStartPos - 50, 50);
                        }
                    }
                }
            }
            frag += "\033[1;34m" + highlight + "\033[0m";
            prevEndPos = termEndPos + 1;
        }

        if (prevEndPos < line.size()) {
            frag += line.substr(prevEndPos, line.size() - prevEndPos >= 50 ? 50 : line.size() - prevEndPos);
        }

        result += frag;
        frag_no++;
    }
    ifs.close();
    return result;
}

void QueryProcessor::processConjunctiveQuery(vector<List*> &lp,int num_terms, pq_top &top_10,information2 &info) {
    unsigned int did=0,d = 0;
    while(did < maxId){
        //get next post from shortest list
        did = nextGEQ(lp[0],did);
        unsigned int max_did = 0;
        int flag = 0;
        //check if there is intersection of doc id did
        for(int i=1; i<num_terms;i++){
            if(lp[i] != nullptr){
                unsigned int d = nextGEQ(lp[i],did);
                max_did = max(d, max_did);
                if(d!=did){
                    flag = 1;

                }
            }
        }
        //if no intersection, start with next minimum did
        if(flag == 1 || max_did == maxId || did == maxId){
            did = (did == maxId)? did : max_did;
            continue;
        }
        //include document in result and compute BM25 score
        else{
            unsigned int ft = 0;
            double score = 0.0;
            unsigned int freq;
            for(int i=0; i<num_terms;i++){
                if(lp[i]){
                    ft = lp[i]->doc_count;
                    freq = getFreq(lp[i],did);
                    double K = BM_b * (get<2>(ptable[did])/BM_avg);
                    K += (1 - BM_b);
                    K *= BM_k1;
                    double temp = (BM_k1 + 1) * freq;
                    temp = temp / (K + freq);
                    temp = (log(BM_N - ft + 0.5) - log(ft+0.5)) * temp;
                    score += temp;
                }
                info[make_tuple(did,i)] = make_tuple(ft,freq);
            }

            if(top_10.size() < 10)
                top_10.push(make_pair(did,score));
            else{
                if(top_10.top().second < score){
                   top_10.pop();
                   top_10.emplace(make_pair(did,score));
                }
            }
            did++;
        }

    }
    for(int i=0; i<num_terms;i++) closeList(lp[i]);
}


void QueryProcessor::processDisjunctiveQuery(vector<List*> &lp, int num_terms,pq_top &top_10,information2 &info) {
    unsigned int did=0;
    unordered_map<unsigned int,double> current_score;

        unsigned int min_did = maxId;
        //for all terms process document;
        for(int i=0; i<num_terms;i++) {
            if (lp[i] != nullptr) {
                unsigned int did = 0;
                while (did < maxId) {
                    unsigned int d = nextGEQ(lp[i],did);
                    if(d == maxId)
                        break;
                    //Compute BM25 score for document and save in hash table;
                    unsigned int ft = lp[i]->doc_count;
                    unsigned int freq = getFreq(lp[i], d);
                    double K = BM_b * (get<2>(ptable[d]) / BM_avg);
                    K += (1 - BM_b);
                    K *= BM_k1;
                    double temp = (BM_k1 + 1) * freq;
                    temp = temp / (K + freq);
                    temp = (log(BM_N - ft + 0.5) - log(ft + 0.5)) * temp;
                    //if prev score exists for same document add the score
                    if(current_score.find(d)!=current_score.end()){
                        current_score[d] += temp;
                    }
                    else{
                        current_score[d]=temp;
                    }
//                    info[d] = make_tuple(current_score[d],freq,ft,BM_avg,get<2>(ptable[d]));
                    info[make_tuple(d,i)] = make_tuple(ft,freq);
                    did = d;
                }

            }
        }
    //All the documents in all the list are evaluated, get top 10 results
    for(int i=0; i<num_terms;i++) closeList(lp[i]);
    for(auto ele : current_score){
        unsigned did = ele.first;
        double score = ele.second;
        //top_10.push(make_pair(did,score));

        if(top_10.size() < 10) top_10.push(make_pair(did,score));
        else{
            if(top_10.top().second < score){
                top_10.pop();
                top_10.push(make_pair(did,score));
            }
        }
    }

}


