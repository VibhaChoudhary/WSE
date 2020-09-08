#include "IndexBuilder.h"

void IndexBuilder::create_postings(filesystem::path p) {
    string line;
    ifstream ifs(p.string()+"//temp.txt");
    string path="D://pages_new//";
    int file = 0;
    string output_path="";
    string start ="WARC-Target-URI:";
    string end ="WARC/1.0";
    int flag = 0;
    while(getline(ifs,line)) {
        if(line.substr(0,start.size()) == start){
            flag = 1;
            docid++;
            if(file % 210 == 0){
                int dir = file/210;
                size_t pos = p.string().find_first_of("CC");
                string root = p.string().substr(pos);
                path =  "D:\\pages_new\\" + root + "\\d_" + to_string(dir);
            }
            file++;
            output_path = path + "\\" + to_string(file) + ".txt";
            //extract url
            line = line.substr(17);
            size_t pos = line.find_first_of("\t\n\v\f\r");
            line = line.substr(0,pos);
            //update page_table with docid and url
            page_table[docid] = make_tuple(line,output_path,0,0);
            //ignore 6 lines of WARC info
            for (int i = 0 ; i < 6; i++){
                getline(ifs,line);
            }
            continue;
        }
        if(line.substr(0,end.size()) == end){
            flag = 0;
        }
        if(flag == 1){
            //parse page and generate postings
            if(!line.empty())
                tokenize(docid,line);
        }
    }
    ifs.close();
}
void IndexBuilder::tokenize(unsigned int docid, string line) {
    string key="";
    string l;
    //parse line
    for(char c : line){
        //if any separator from the list is found save the current buffered word
        if(find(separator.begin(),separator.end(),c) == separator.end()){
            if(!ispunct(c) && isprint(c) && isalnum(c)){
                c = tolower(c);
                key += c;
            }
        }
        else{
            if(is_valid_key(key)){
                //create term id
                if(term_id.find(key) == term_id.end()){
                    term_id[key] = curr_term_id;
                    curr_term_id++;
                }
                //increase doc length of the docid
                get<2>(page_table[docid])++;

                postings[make_pair(term_id[key],docid)]++;
                postings_count++;
                //write out the intermediate postings to file
                if(postings_count != 0 && postings_count % postings_buffer_size == 0){
                    string path = temp_path.string();
                    path = path + "\\" + to_string(postings_file_num);

                    write_map_postings(postings,fs::path(path));
                }

            }
            key="";
        }
    }

}
bool IndexBuilder::is_valid_key(string &key) {
    int alpha_count=0;
    //term that only has numbers is not considered
    for(char c: key){
        if(isalpha(c))
            alpha_count++;
    }
    //term should not be empty, should not be greater than max chars and should have more alphabets than digits
    if(!key.empty() && key.size() <= max_term_size && alpha_count > 0){
        return true;
    }
    return false;
}
void IndexBuilder::merge_postings() {
    string path = temp_path.string();
    path = path + "//merge";
    fs::create_directory(fs::path(path));
    filesystem::path output = fs::path(path);
    postings_file_num = 59;
    //if atleast one intermediate posting file is created
    if(postings_file_num != 1){
        int start = postings_file_num - 1;
        int run = 1;
        filesystem::path input = temp_path;
        while(start > 0){
            output = fs::path(path+"//"+to_string(run));
            fs::create_directory(output);
            for(int i=1;i<=start;i+=merge_degree){
                k_way_merge(input,output,i,start);
            }
            if(start > merge_degree){
                start = start % merge_degree == 0?(start/merge_degree):(start/merge_degree)+1;
            }
            else start = start/merge_degree;
            input = output;
            run++;
        }
    }
   //if no intermediate posting file is created, all postings are still in memory directly call create index.
    else{
        for(auto ele : postings){
            //create_index(get<0>(ele),get<1>(ele),get<2>(ele));
            //create_index(ele.first.first,ele.first.second,ele.second);
            create_index(ele.first.first,ele.first.second,ele.second);
        }
    }
    cout<<"Saving data structures to file..."<<endl;
    if(temp_postings.size() > 0){
        write_index();
    }
    inv.close();
    lex.close();
    //fs::remove_all(fs::path(path));
}
void IndexBuilder::k_way_merge(filesystem::path input_dir, filesystem::path output_file,int file_index,int rem) {
    //open k file pointers
    ifstream ifs[merge_degree];
    int count = 0;
    for(int i = 0; i < merge_degree; i++){
        string path = input_dir.string();
        path = path + "//" + to_string(file_index + i);
        if(fs::exists(fs::path(path))){
            count++;
            ifs[i].open(path,ifstream::in|ifstream::binary);
        }
        else
            break;
    }
    //place intial data from k files into heap
    priority_queue<posting2,vector<posting2>,compare> heap;
    for(int i = 0; i < count; i++)
    {
        if(!ifs[i].eof()){

            unsigned long long int tid;
            unsigned int docid;
            unsigned short int freq;
            ifs[i].read(reinterpret_cast<char *>(&tid), sizeof(tid));
            ifs[i].read(reinterpret_cast<char *>(&docid), sizeof(docid));
            ifs[i].read(reinterpret_cast<char *>(&freq), sizeof(freq));

            heap.push(make_tuple(i,tid,docid,freq));
        }

    }
    //create output merge file for current run
    int f = ((file_index) / merge_degree) + 1;
    ofstream ofs(string(output_file.string()+"//"+ to_string(f)),ofstream::out|ofstream::binary);
    //keep merging while heap is full
    while(heap.size() > 0){
        auto ele  = heap.top();
        heap.pop();
        int index = get<0>(ele);
        //string term = get<1>(ele);
        //unsigned char term_size = term.size();
        unsigned long long int tid = get<1>(ele);
        unsigned int docid = get<2>(ele);
        unsigned short int freq = get<3>(ele);

        //if last phase of merging directly go to create index
        if(rem <= merge_degree){
            //create_index(term,docid,freq);
            create_index(tid,docid,freq);
        }
        else{

            ofs.write((char*)(&tid), sizeof(tid));
            ofs.write((char*)(&docid), sizeof(docid));
            ofs.write((char*)(&freq), sizeof(freq));
        }
        //push next posting into heap from the file which had minimum.
        if(!ifs[index].eof()){
            //get next posting from the file

            unsigned long long int tid;
            unsigned int docid;
            unsigned short int freq;
            ifs[index].read(reinterpret_cast<char *>(&tid), sizeof(tid));
            ifs[index].read(reinterpret_cast<char *>(&docid), sizeof(docid));
            ifs[index].read(reinterpret_cast<char *>(&freq), sizeof(freq));

            heap.push(make_tuple(index,tid,docid,freq));
        }
        else{
            ifs[index].close();
            string path = input_dir.string();
            path = path + "//" + to_string(file_index + index);
            fs::remove(fs::path(path));
        }
    }
    ofs.close();
}
void IndexBuilder::create_index(unsigned long long int term, unsigned int docid, unsigned short int freq){
    static unsigned long int size = 0;
    static unsigned long long int postings_count = 0;
    static long int last_term =-1;
    static long int curr_term =-1;
    if(last_term == -1){
        inv.open(string(data_path.string()+"//inverted_index"),ofstream::out|ofstream::binary);
        lex.open(string(data_path.string()+"//lexicon"),ofstream::out);
    }
    postings_count++;
    //if new term save posting
    if(last_term != term){
        if(last_term!=-1)
            write_index();
        temp_postings.push_back(make_tuple(term, docid, freq));
        last_term = term;
    }
    else{
        //if same term and same docid increase frequency
        auto prev_posting = temp_postings.back();
        if(get<0>(prev_posting) == term and get<1>(prev_posting)==docid)
            get<2>(prev_posting) += freq;
            //if same term but different docid create new posting
        else
            temp_postings.push_back(make_tuple(term,docid,freq));size++;

    }
}
void IndexBuilder::vb_encode(unsigned int n,vector<unsigned char> &result){
    while(true){
        result.push_back(n % 128);
        if(n <128) break;
        n = n / 128;
    }
    result[result.size()-1]+=128;
}


void IndexBuilder::write_index() {
    int i = 0;
    unsigned long long int start_byte = curr_byte_offset;
    unsigned int num_bytes = 0;
    unsigned long long int base = 0;
    unsigned long long int tid = get<0>(temp_postings[0]);
    while(i<temp_postings.size()){
        vector<unsigned char> result;
        //encode docid
        vb_encode(get<1>(temp_postings[i])-base,result);
        curr_byte_offset += result.size() * sizeof(unsigned char);
        num_bytes += result.size();
        for(unsigned char ele : result)
            inv.write((char*)&ele,sizeof(ele));
        result.clear();
        //encode freq;
        vb_encode(get<2>(temp_postings[i]),result);
        curr_byte_offset += result.size() * sizeof(unsigned char);
        num_bytes += result.size();
        for(unsigned char ele : result)
            inv.write((char*)&ele,sizeof(ele));
        base = get<1>(temp_postings[i]);
        i++;
        if(i > temp_postings.size()) break;

    }
    lexicon[tid] = make_pair(start_byte,num_bytes);
    temp_postings.clear();
    write_lexicon();
}
void IndexBuilder::write_page_table() {
    string path = data_path.string();
    path = path + "//" + "page_table";
    ofstream ofs(path,ofstream::out);
    for(auto ele : page_table){
        ofs << ele.first <<" ";
        ofs << get<0>(ele.second)<<" ";
        ofs << get<1>(ele.second)<<" ";
        ofs << get<2>(ele.second)<<" ";
        ofs << get<3>(ele.second)<<"\n";
    }
    ofs.close();
    page_table.clear();
}
void IndexBuilder::write_lexicon() {
    for(auto ele : lexicon){
        lex << ele.first <<" ";
        lex << ele.second.first<<" ";
        lex << ele.second.second<<"\n";
    }
    lexicon.clear();
}
void IndexBuilder::write_term_id() {
    ofstream ofs;
    ofs.open(string(data_path.string()+"//term_id"),ofstream::out);
    for(auto ele : term_id){
        ofs<<ele.first<<" ";
        ofs<<ele.second<<"\n";
    }
    ofs.close();
    term_id.clear();
}
void IndexBuilder::write_map_postings(map<key, unsigned short, struct compare_keys>& postings,filesystem::path file_path) {

    ofstream ofs(string(file_path.string()),ofstream::out|ofstream::binary);
    for(auto ele : postings) {
        //create posting
        unsigned long long int term = ele.first.first;
        unsigned int docid = ele.first.second;
        unsigned short int freq = ele.second;
        ofs.write((char *) &term, sizeof(term));
        ofs.write((char *) (&docid), sizeof(docid));
        ofs.write((char *) (&freq), sizeof(freq));
    }
    ofs.close();
    postings_file_num++;
    postings.clear();
}

unsigned long long int IndexBuilder::get_terms_count() {
    return curr_term_id-1;
}
unsigned long long int IndexBuilder::get_postings_count() {
    return postings_count;
}
unsigned long int IndexBuilder::get_last_file_num() {
    return postings_file_num;
}


