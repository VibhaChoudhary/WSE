#include "src/IndexBuilder.h"
#include <chrono>
#define PAGES "sample_pages_2"
#define POSTINGS "sample_postings_2"
#define DATA "sample_data_2"
#define MAX_TERM_SIZE 30
#define POSTINGS_BUFFER_SIZE 6000
#define MERGE_DEGREE 10

typedef std::chrono::duration<float> float_seconds;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    IndexBuilder indexBuilder;
    if(argc<4){
        cout<<"No paths provided, running with sample data.."<<endl;
        indexBuilder.data_path = fs::path(DATA);
        indexBuilder.temp_path = fs::path(POSTINGS);
        indexBuilder.page_path = fs::path(PAGES);
    }
    else{
        indexBuilder.data_path = fs::path(argv[3]);
        indexBuilder.temp_path = fs::path(argv[2]);
        indexBuilder.page_path = fs::path(argv[1]);
    }
    fs::remove_all(fs::path(indexBuilder.temp_path));
    fs::create_directory(fs::path(indexBuilder.temp_path));
    fs::remove_all(fs::path(indexBuilder.data_path));
    fs::create_directory(fs::path(indexBuilder.data_path));

    indexBuilder.merge_degree=MERGE_DEGREE;
    indexBuilder.postings_buffer_size=POSTINGS_BUFFER_SIZE;
    indexBuilder.max_term_size=MAX_TERM_SIZE;
    int create_posting_time = 0;
    unsigned int page_count=0;
    cout<<"Creating intermediate postings..."<<endl;
    auto start = std::chrono::system_clock::now();
    try {
        //Go recursively through the pages.
        auto it = fs::recursive_directory_iterator(indexBuilder.page_path);
        for(;it != fs::recursive_directory_iterator(); ++it ) {
            if(it->path().extension() == ".txt"){
                int doc_length=0;
                page_count++;
                //cout<<"working on page"<<page_count<<endl;
                //create intermediate postings
                indexBuilder.create_postings(page_count,it->path(),doc_length);
            }
        }
        //write out the remaining postings to file
        if(!indexBuilder.postings.empty()){
            if(indexBuilder.get_last_file_num()!=1){
                string path = indexBuilder.temp_path.string();
                path = path + "\\" + to_string(indexBuilder.get_last_file_num());
                indexBuilder.write_postings(indexBuilder.postings,fs::path(path));
            }
        }
        auto end = std::chrono::system_clock::now();
        auto create_time = std::chrono::duration_cast<float_seconds>(end-start);
        //print_postings(indexBuilder);
        cout<<"Doing merge on postings..."<<endl;
        start = std::chrono::system_clock::now();
        //merge postings
        indexBuilder.merge_postings();
        end = std::chrono::system_clock::now();
        auto merge_time = std::chrono::duration_cast<float_seconds>(end-start);
        cout<<"Building index..."<<endl;
        cout<<"Saving data structures to file..."<<endl;
        //write page_table
        indexBuilder.write_page_table();
        //write page_table
        indexBuilder.write_lexicon();
        //write term_id
        indexBuilder.write_term_id();

        //statistics
        cout<<"***Data Statitics****"<<endl;
        cout<<"Total pages indexed:"<<page_count<<endl;
        cout<<"Total postings created:"<<indexBuilder.get_postings_count()<<endl;
        cout<<"Total unique terms:"<<indexBuilder.get_terms_count()<<endl;
        cout<<"***Time Statitics***"<<endl;
        cout<<"Time for create postings:"<<create_time.count()<<" seconds"<<endl;
        cout<<"Time for merge postings:"<<merge_time.count()<<" seconds"<<endl;
        cout<<"Total:"<<merge_time.count() + create_time.count()<<" seconds"<<endl;
    }
    catch (const fs::filesystem_error& err) {
        std::cerr << "filesystem error! " << err.what() << '\n';
    }
    catch (const std::exception& ex) {
        std::cerr << "general exception: " << ex.what() << '\n';
    }
    return 0;
}