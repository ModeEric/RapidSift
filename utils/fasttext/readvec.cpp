#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Fast memory-mapped version
std::unordered_map<std::string, std::vector<float>> get_embeds(const std::string& filename) {
    // Open file
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return {};
    }
    
    // Get file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        close(fd);
        std::cerr << "Error: Could not get file stats" << std::endl;
        return {};
    }
    
    size_t file_size = sb.st_size;
    if (file_size == 0) {
        close(fd);
        return {};
    }
    
    // Memory map the file
    char* file_data = static_cast<char*>(mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (file_data == MAP_FAILED) {
        close(fd);
        std::cerr << "Error: Memory mapping failed" << std::endl;
        return {};
    }
    
    // Close file descriptor (mmap keeps the mapping)
    close(fd);
    
    std::unordered_map<std::string, std::vector<float>> embeds;
    embeds.reserve(1000000); // Pre-allocate space for ~1M entries
    
    // Parse the memory-mapped file
    size_t pos = 0;
    while (pos < file_size) {
        // Find end of current line
        size_t line_start = pos;
        while (pos < file_size && file_data[pos] != '\n') {
            pos++;
        }
        
        if (pos == line_start) {
            pos++; // Skip empty lines
            continue;
        }
        
        // Create string view of the line (no copying!)
        std::string_view line(file_data + line_start, pos - line_start);
        
        // Find first space
        size_t space_pos = line.find(' ');
        if (space_pos == std::string_view::npos) {
            pos++; // Move to next line
            continue;
        }
        
        // Extract word (no copying!)
        std::string_view word_view = line.substr(0, space_pos);
        std::string word(word_view); // Only copy when creating the string
        
        // Parse vector values
        std::vector<float> vec;
        vec.reserve(300); // Most word vectors are 300-dimensional
        
        size_t vec_start = space_pos + 1;
        while (vec_start < line.length()) {
            // Skip whitespace
            while (vec_start < line.length() && std::isspace(line[vec_start])) {
                vec_start++;
            }
            if (vec_start >= line.length()) break;
            
            // Find end of number
            size_t vec_end = vec_start;
            while (vec_end < line.length() && !std::isspace(line[vec_end])) {
                vec_end++;
            }
            
            // Convert to float
            if (vec_end > vec_start) {
                try {
                    std::string_view num_str = line.substr(vec_start, vec_end - vec_start);
                    float value = std::stof(std::string(num_str));
                    vec.push_back(value);
                } catch (const std::exception&) {
                    // Skip invalid numbers
                }
            }
            
            vec_start = vec_end;
        }
        
        // Use move semantics to avoid copying
        embeds.emplace(std::move(word), std::move(vec));
        
        pos++; // Move to next line
    }
    
    // Unmap the file
    munmap(file_data, file_size);
    
    return embeds;
}