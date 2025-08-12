#pragma once

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "src/common/types.hpp"

namespace net {

struct PlayerInfo {
  common::PlayerId playerId;
  asio::ip::udp::endpoint endpoint;
  std::chrono::steady_clock::time_point lastSeen;
};

class ClientManager {
public:
  ClientManager() : nextId_(0) {}

  // Return existing player id for endpoint or assign a new one.
  common::PlayerId getOrCreatePlayerId(const asio::ip::udp::endpoint &endpoint) {
    const std::string key = endpointToString(endpoint);
    std::lock_guard<std::mutex> lock(mutex_);

    if (auto it = endpointToId_.find(key); it != endpointToId_.end()) {
      auto infoIt = idToInfo_.find(it->second);
      if (infoIt != idToInfo_.end()) {
        infoIt->second.lastSeen = std::chrono::steady_clock::now();
      }
      return it->second;
    }

    const common::PlayerId newId = ++nextId_;
    endpointToId_[key] = newId;
    idToInfo_.emplace(newId, PlayerInfo{newId, endpoint, std::chrono::steady_clock::now()});
    return newId;
  }

  std::optional<common::PlayerId> findPlayerId(const asio::ip::udp::endpoint &endpoint) const {
    const std::string key = endpointToString(endpoint);
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = endpointToId_.find(key); it != endpointToId_.end()) return it->second;
    return std::nullopt;
  }

  std::optional<PlayerInfo> getPlayerInfo(common::PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = idToInfo_.find(playerId); it != idToInfo_.end()) return it->second;
    return std::nullopt;
  }

  std::vector<asio::ip::udp::endpoint> listEndpoints() const {
    std::vector<asio::ip::udp::endpoint> endpoints;
    std::lock_guard<std::mutex> lock(mutex_);
    endpoints.reserve(idToInfo_.size());
    for (const auto &kv : idToInfo_) {
      endpoints.push_back(kv.second.endpoint);
    }
    return endpoints;
  }

  static std::string endpointToString(const asio::ip::udp::endpoint &endpoint) {
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
  }

  std::size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return idToInfo_.size();
  }

private:
  mutable std::mutex mutex_;
  std::unordered_map<std::string, common::PlayerId> endpointToId_;
  std::unordered_map<common::PlayerId, PlayerInfo> idToInfo_;
  std::atomic<common::PlayerId> nextId_;
};

} // namespace net

