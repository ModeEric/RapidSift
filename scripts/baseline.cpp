#include "../utils/fasttext/readvec.cpp"

int main(){
    std::unordered_map<std::string,std::vector<float> >  embeds = get_embeds("wiki-news-300d-1M-1.vec");
    return 0;
}