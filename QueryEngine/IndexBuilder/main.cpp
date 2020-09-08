#include "src/IndexBuilder.h"
#include <chrono>
#define PAGES "pages"
#define POSTINGS "sample_postings"
#define DATA "data"
#define MAX_TERM_SIZE 40
#define POSTINGS_BUFFER_SIZE 1000000
#define MERGE_DEGREE 16


typedef std::chrono::duration<float> float_seconds;
using namespace std::chrono;

int main(int argc, char* argv[]) {
    IndexBuilder indexBuilder;

    unsigned int page_count = 0;
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
    indexBuilder.merge_degree = MERGE_DEGREE;
    indexBuilder.postings_buffer_size = POSTINGS_BUFFER_SIZE;
    indexBuilder.max_term_size = MAX_TERM_SIZE;
    cout<<"Creating intermediate postings..."<<endl;
    auto start = std::chrono::system_clock::now();
    try {
        //Go recursively through the pages.

        auto it = fs::directory_iterator(indexBuilder.page_path);

        for(;it != fs::directory_iterator(); ++it ) {
            indexBuilder.create_postings(it->path());
        }

        //write out the remaining postings to file
        if(!indexBuilder.postings.empty()){
            if(indexBuilder.get_last_file_num()!=1){
                string path = indexBuilder.temp_path.string();
                path = path + "\\" + to_string(indexBuilder.get_last_file_num());
                indexBuilder.write_map_postings(indexBuilder.postings,fs::path(path));
            }
        }
        auto create_time = std::chrono::duration_cast<float_seconds>(std::chrono::system_clock::now()-start);
        cout<<"Total pages :"<<indexBuilder.docid<<endl;
        cout<<"Time for create postings:"<<create_time.count()<<" seconds"<<endl;
        cout<<"writing out page_table.."<<endl;
        //write page_table
        indexBuilder.write_page_table();
        //write term_mapping
        cout<<"writing out term_mapping.."<<endl;
        indexBuilder.write_term_id();
        //merge postings
        cout<<"Doing merge on postings..."<<endl;
        start = std::chrono::system_clock::now();
        indexBuilder.merge_postings();
        auto merge_time = std::chrono::duration_cast<float_seconds>(std::chrono::system_clock::now()-start);
        //statistics
        cout<<"***Data Statistics****"<<endl;
        cout<<"Total pages indexed:"<<indexBuilder.docid<<endl;
        cout<<"Total postings created:"<<indexBuilder.get_postings_count()<<endl;
        cout<<"Total unique terms:"<<indexBuilder.get_terms_count()<<endl;
        cout<<"***Time Statistics***"<<endl;
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