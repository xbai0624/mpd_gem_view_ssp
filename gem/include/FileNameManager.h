#ifndef FILE_NAME_MANAGER_H
#define FILE_NAME_MANAGER_H

#include <string> 
#include <map> 

class FileNameManager { 
public:
  static FileNameManager& getInstance() {
    static FileNameManager instance;
    return instance;
  }

  // Set a file name for a specific key 
  void setFileName(const std::string& key, const std::string& fileName) {
    fileNames[key] = fileName; 
  }

  // Get a file name for a specific key 
  const std::string& getFileName(const std::string& key) const {
    auto it = fileNames.find(key); 
    if (it != fileNames.end()) { 
	return it->second; 
    } else {
	throw std::runtime_error("File name for kay " + key + " not found");
    }
  }

private:
  FileNameManager() {}
  FileNameManager(const FileNameManager&) = delete;
  FileNameManager& operator=(const FileNameManager&) = delete;

  std::map<std::string, std::string> fileNames; // Map to store file names
}; 
    
#endif // FILE_NAME_MANAGER_H
