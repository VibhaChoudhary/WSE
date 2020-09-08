#include "src/QueryProcessor.h"

#define DATA "indexer/data"
#define PAGE_TABLE DATA"/page_table"
#define LEXICON DATA"/lexicon"
#define TERM_MAPPING DATA"/term_id"
#define INVERTED_INDEX DATA"/inverted_index"


int main(int argc, char* argv[]) {
    QueryProcessor queryProcessor;
    //if data paths are provided use those else use sample data paths
    if(argc<4){
        cout<<"No paths provided, running with sample data.."<<endl;
        queryProcessor.page_table = fs::path(PAGE_TABLE);
        queryProcessor.lexicon = fs::path(LEXICON);
        queryProcessor.term_mapping = fs::path(TERM_MAPPING);
        queryProcessor.inverted_index = fs::path(INVERTED_INDEX);
    }
    else{
        queryProcessor.page_table = fs::path(argv[1]);
        queryProcessor.lexicon = fs::path(argv[2]);
        queryProcessor.term_mapping = fs::path(argv[3]);
        queryProcessor.inverted_index = fs::path(argv[4]);
    }
    queryProcessor.init();

    while(true) {
        string query;
        bool if_conjuctive = 1;
        cout << "Enter query:" << endl;
        getline(cin, query);
        cout << "Press 1 for Conjunctive and 0 for Disjunctive.." << endl;
        cin >> if_conjuctive;
        query_result result = queryProcessor.processQuery(query, if_conjuctive);
        if (result.empty()) cout <<"No results found"<<endl;
        else {
            for (int i=result.size()-1;i>=0;i--) {
                auto ele = result[i];
                cout <<endl<< "URL -> " << get<0>(ele) <<endl;
                cout << "Location -> " << get<1>(ele) << endl;
                cout << "Score -> " << get<3>(ele) <<" | ";
                cout << "Doc length -> " << get<4>(ele) <<" | ";
                cout << "Doc avg length -> " << get<5>(ele) <<" | ";
                term_information tinfo = get<6>(ele);
                for(auto key : tinfo){
                    cout << "("<<get<0>(key)<<", ft, tf) -> ("<<get<1>(key)<<", "<<get<2>(key)<<")"<<" | ";
                }
                cout <<  endl;
                cout<<"Snippet ->"<<endl;
                cout <<  get<2>(ele)<< endl;
            }
        }
        cout<<"Press esc to exit"<<endl;
        char ch = getchar();
        if(ch == 27) break;
    }

    return 0;
}