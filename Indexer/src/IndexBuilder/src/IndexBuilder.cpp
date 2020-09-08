#include "IndexBuilder.h"
/*
create postings for each docid in the path 
*/
void IndexBuilder::create_postings(unsigned int docid, filesystem::path p,int & doc_length) {
    string url,line;
    ifstream ifs(p);
    if(ifs) {
        getline(ifs,url);
        //extract url
        url = url.substr(17,string::npos);
        //update page_table with docid and url
        page_table[docid] = make_tuple(url,0,0);
        //ignore 6 lines of WARC info
        for (int i = 0 ; i < 6; i++){
            getline(ifs,line);
        }
        //parse page and generate postings
        while(getline(ifs,line)) {
            if(!line.empty())
                tokenize(docid,line);
        }
    }
    ifs.close();
}
/*
merge postings
*/
void IndexBuilder::merge_postings() {
    string path = temp_path.string();
    //create merge directory
    path = path + "\\merge";
    fs::create_directory(fs::path(path));
    filesystem::path output = fs::path(path);
    //if only one file directly create index
    if(postings_file_num != 1){
        int start = postings_file_num - 1;
        //current run
        int run = 1;
        filesystem::path input = temp_path;
        while(start > 0){
                output = fs::path(path+"\\"+to_string(run));
                fs::create_directory(output);
                for(int i=1;i<=start;i+=merge_degree){
                    //merge sort files
                    k_way_merge(input,output,i,start%merge_degree);
                }
                start = start / merge_degree;
                input = output;
                run++;
            }
    }
    else{
        for(auto ele :postings){
            create_index(ele.first.first,ele.first.second,ele.second);
        }
    }
    //write out remaining postings to index file
    if(temp_postings.size() > 0){
        write_temp_postings();
    }

}
/*
create index called for each posting from merge step.Buffers the posting and write to file
*/
void IndexBuilder::create_index(string term, unsigned int docid, unsigned short int freq){
    if(term_id.find(term) == term_id.end()){
        //if first term
        if(curr_term_id == 1){
            //open inverted index file
            inv.open(string(data_path.string()+"\\inverted_index"),ofstream::out|ofstream::binary);
            term_id[term] = curr_term_id;
            temp_postings.push_back(make_tuple(term_id[term], docid, freq));
            curr_term_id++;
            return;
        }
        //if buffer size reached write out postings to index
        if(temp_postings.size()!=0 && temp_postings.size() % postings_buffer_size == 0) {
            write_temp_postings();
        }
        term_id[term] = curr_term_id;
        temp_postings.push_back(make_tuple(term_id[term], docid, freq));
        curr_term_id++;
    }
    else{
        auto prev_posting = temp_postings.back();
        if(get<0>(prev_posting) == term_id[term] and get<1>(prev_posting)==docid)
            get<2>(prev_posting) += freq;
        else
            temp_postings.push_back(make_tuple(term_id[term],docid,freq));
    }
}

void IndexBuilder::tokenize(unsigned int docid, string line) {
    string key="";
    string l;
    //parse line
    for(char c : line){
        //if any separator from the list is found save the current buffered word
        if(find(separator.begin(),separator.end(),c) == separator.end()){
            c=tolower(c);
            key+=c;
        }
        else{
            if(is_valid_key(key)){
                get<1>(page_table[docid])++;
                auto postings_key = make_pair(key,docid);
                   if(postings.find(postings_key)!=postings.end()){
                        postings[postings_key]++;
                   }
                    else {
                        postings[postings_key]=1;
                        postings_count++;
                    }

                //write out the intermediate postings to file
                if(postings_count != 0 && postings_count % postings_buffer_size == 0){
                    string path = temp_path.string();
                    path = path + "\\" + to_string(postings_file_num);
                    write_postings(postings,fs::path(path));
                }

            }
            key="";
        }
    }
}
bool IndexBuilder::is_valid_key(string &key) {
        int alpha_count=0;
        //term that only has numbers is not considered
        for(char c: key){if(isalpha(c)) alpha_count++;}
        string new_key = "";
        for(char c: key){if(isalnum(c)) new_key+=c;}
        key = new_key;
        //term should not be empty, should not be greater than max chars and should have more alphabets than digits
        if(!key.empty() && key.size() <= max_term_size && alpha_count > 0){
            return true;
        }
        return false;
    }

void IndexBuilder::vb_encode(unsigned int n,vector<unsigned char> &result){
    while(n >= 128){
        result.push_back((128 + n % 128) & 0xFF);
        n = n / 128;
    }
    result.push_back(n & 0xFF);
}
void IndexBuilder::vb_decode(vector<unsigned char> input,unsigned int &n){
    int i=0;
    int d=1;
    while(input[i] >= 128){
        n = n + d * (input[i] - 128);
        d = d * 128;
        i = i + 1;
    }
    n = n + d * input[i];
}
void IndexBuilder::k_way_merge(filesystem::path input_dir, filesystem::path output_file,int file_index,int rem) {
    ifstream ifs[merge_degree];
    int f = (file_index / merge_degree) + 1;
    int count = 0;
    //create k file pointers
    for(int i = 0; i < merge_degree; i++){
        string path = input_dir.string();
        path = path + "\\" + to_string(file_index + i);
        if(fs::exists(fs::path(path))){
            count++;
            ifs[i].open(path,ifstream::in|ifstream::binary);
        }
        else
            break;
    }
    //output file for each run
    ofstream ofs(string(output_file.string()+"\\"+ to_string(f)),ofstream::out|ofstream::binary);
    //priority queue of postings
    priority_queue<posting,vector<posting>,compare> heap;
    for(int i = 0; i < count; i++)
    {
        if(!ifs[i].eof()){
            byte term_size;
            ifs[i].read(reinterpret_cast<char *>(&term_size), sizeof(term_size));
            char* term = new char[(int)term_size + 1];
            ifs[i].read(&term[0], (int)term_size);
            term[(int)term_size] = '\0';
            unsigned int docid;
            unsigned short int freq;
            ifs[i].read(reinterpret_cast<char *>(&docid), sizeof(docid));
            ifs[i].read(reinterpret_cast<char *>(&freq), sizeof(freq));
            string trm=string(term);
            heap.emplace(make_tuple(i,trm,docid,freq));
        }
    }
    while(heap.size() > 0){
        auto ele  = heap.top();
        heap.pop();
        int index = get<0>(ele);
        string term = get<1>(ele);
        byte term_size = static_cast<byte>(term.size());
        unsigned int docid = get<2>(ele);
        unsigned short int freq = get<3>(ele);
        //if final merge step start creating index
        if(rem < 2){
            create_index(term,docid,freq);
        }
        else{
            //write out to merge file
            ofs.write((char*)&term_size, sizeof(term_size));
            ofs.write(term.c_str(), term.size());
            ofs.write((char*)(&docid), sizeof(docid));
            ofs.write((char*)(&freq), sizeof(freq));
        }

        if(!ifs[index].eof()){
            //get next posting from the file
            byte term_size;
            ifs[index].read(reinterpret_cast<char *>(&term_size), sizeof(term_size));
            char* term = new char[(int)term_size + 1];
            ifs[index].read(&term[0], (int)term_size);
            term[(int)term_size] = '\0';
            unsigned int docid;
            unsigned short int freq;
            ifs[index].read(reinterpret_cast<char *>(&docid), sizeof(docid));
            ifs[index].read(reinterpret_cast<char *>(&freq), sizeof(freq));
            string trm = string(term);
            heap.emplace(make_tuple(index,trm,docid,freq));
        }
        else{
            //close temp file and remove from directory
            ifs[index].close();
            string path = input_dir.string();
            path = path + "\\" + to_string(file_index + index);
            fs::remove(fs::path(path));
        }
    }
    ofs.close();
    inv.close();
}

unsigned int IndexBuilder::get_terms_count() {
    return curr_term_id-1;
}
unsigned int IndexBuilder::get_postings_count() {
    return postings_count;
}
unsigned int IndexBuilder::get_last_file_num() {
    return postings_file_num;
}
//write index structures to file
void IndexBuilder::write_postings(map<key, unsigned short int,compare_keys> &postings,filesystem::path file_path) {
    ofstream ofs(file_path,ofstream::out|ofstream::binary);
    for(auto ele : postings) {
        //create posting <term_size_1byte,term,docid_4byte,freq_2byte>
        string term = ele.first.first;
        byte term_size = static_cast<byte>(term.size());
        unsigned int docid = ele.first.second;
        unsigned short int freq = ele.second;
        ofs.write((char *) &term_size, sizeof(byte));
        ofs.write(term.c_str(), term.size());
        ofs.write((char *) (&docid), sizeof(docid));
        ofs.write((char *) (&freq), sizeof(freq));
    }
    ofs.close();
    postings_file_num++;
    postings.clear();
}
void IndexBuilder::write_page_table() {
    string path = data_path.string();
    path = path + "\\" + "page_table";
    ofstream ofs(path,ofstream::out);
    for(auto ele : page_table){
        ofs << ele.first <<" ";
        ofs << get<0>(ele.second)<<" ";
        ofs << get<1>(ele.second)<<" ";
        ofs << get<2>(ele.second)<<"\n";
    }
    ofs.close();
}
void IndexBuilder::write_lexicon() {
    string path = data_path.string();
    path = path + "\\" + "lexicon";
    ofstream ofs(path,ofstream::out);
    for(auto ele : lexicon){
        ofs << ele.first <<" ";
        ofs << ele.second.first<<" ";
        ofs << ele.second.second<<"\n";
    }
    ofs.close();
}
void IndexBuilder::write_term_id() {
    string path = data_path.string();
    path = path + "\\" + "term_id";
    ofstream ofs(path,ofstream::out);
    for(auto ele : term_id){
        ofs << ele.first <<" ";
        ofs << ele.second<<"\n";
    }
    ofs.close();
}
void IndexBuilder::write_temp_postings() {
    int i = 0;
    while(i<temp_postings.size()){
        int tid = get<0>(temp_postings[i]);
        unsigned int start_byte = curr_byte_offset;
        unsigned int num_bytes = 0;
        unsigned int base = 0;
        while(get<0>(temp_postings[i])==tid){
            vector<unsigned char> result;
            //encode docid
            vb_encode(get<1>(temp_postings[i])-base,result);
            curr_byte_offset += result.size() * sizeof(byte);
            num_bytes+=result.size();
            for(unsigned char ele : result)
                inv.write((char*)&ele,sizeof(char));
            result.clear();
            //encode freq;
            vb_encode(get<2>(temp_postings[i]),result);
            curr_byte_offset += result.size() * sizeof(byte);
            num_bytes+=result.size();
            for(unsigned char ele : result)
                inv.write((char*)&ele,sizeof(char));
            base = get<1>(temp_postings[i]);
            i++;
            if(i==283323)
            {
                cout<<"";
            }
        }
        lexicon[tid] = make_pair(start_byte,num_bytes);
    }
    temp_postings.clear();
}
