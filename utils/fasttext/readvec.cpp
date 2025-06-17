std::vector<std::unoredered_map<char*,std::vector<float>> get_embeds(char* filename){
    std::ifstream istream(filename);
    std::string s;
    std::unoredered_map<char*,std::vector<float>> embeds;
    while (std::getline(istream,s)){
        std::istringstream iss(s);
        std::string word;
        std::vector<float> vec;
        iss >> word;
        while (iss >> f){
            vec.push_back(f);
        }
        embeds[word] = vec;
    }
    return embeds;
}