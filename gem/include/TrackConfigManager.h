#ifndef TRACK_CONFIG_MANAGER_H
#define TRACK_CONFIG_MANAGER_H

#include <string> 

class TrackConfigManager { 
public:
  static TrackConfigManager& getInstance() {
    static TrackConfigManager instance;
    return instance;
  }

  void setTrackConfig(const std::string& trackConfig) {
    this->trackConfig = trackConfig;
  }

  const std::string& getTrackConfig() const {
    return trackConfig;
  }

private:
  TrackConfigManager():trackConfig("") {}
  TrackConfigManager(const TrackConfigManager&) = delete;
  TrackConfigManager& operator=(const TrackConfigManager&) = delete;

  std::string trackConfig;
}; 
    
#endif // TRACK_CONFIG_MANAGER_H
