/**
 * @brief Debug stream reading issue
 */

#include <iostream>
#include <sstream>
#include <string>

int main() {
    std::string json_content = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"processId\":null}}";
    std::string lsp_message = "Content-Length: " + std::to_string(json_content.length()) + "\r\n\r\n" + json_content;
    
    std::cout << "JSON content length: " << json_content.length() << std::endl;
    std::cout << "Full message: [" << lsp_message << "]" << std::endl;
    std::cout << "Full message length: " << lsp_message.length() << std::endl;
    
    std::istringstream input(lsp_message);
    
    // Read Content-Length header
    std::string line;
    while (std::getline(input, line)) {
        std::cout << "Read line: [" << line << "]" << std::endl;
        
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            std::cout << "Found empty line, breaking" << std::endl;
            break;
        }
        
        if (line.find("Content-Length:") == 0) {
            std::string length_str = line.substr(15);
            std::cout << "Content-Length value: [" << length_str << "]" << std::endl;
            
            // Trim whitespace
            size_t start = length_str.find_first_not_of(" \t");
            if (start != std::string::npos) {
                length_str = length_str.substr(start);
                size_t end = length_str.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    length_str = length_str.substr(0, end + 1);
                }
            }
            
            std::cout << "Trimmed length: [" << length_str << "]" << std::endl;
            size_t content_length = std::stoul(length_str);
            std::cout << "Parsed content length: " << content_length << std::endl;
            
            // Read JSON payload
            std::string payload(content_length, '\0');
            input.read(&payload[0], content_length);
            
            std::cout << "Read " << input.gcount() << " characters" << std::endl;
            std::cout << "Payload: [" << payload << "]" << std::endl;
            
            return 0;
        }
    }
    
    return 1;
}
