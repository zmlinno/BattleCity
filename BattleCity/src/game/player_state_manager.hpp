#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "src/common/types.hpp"

namespace game {

class PlayerStateManager {
public:
  PlayerStateManager() = default;

  // Initialize player state
  void initializePlayer(common::PlayerId playerId, double startX = 0.0, double startY = 0.0) {
    std::lock_guard<std::mutex> lock(mutex_);
    playerStates_[playerId] = common::PlayerState{
        startX, startY, 0.0, 0.0, 0.0, 100, true
    };
    lastUpdate_[playerId] = std::chrono::steady_clock::now();
  }

  // Update player state
  void updatePlayerState(common::PlayerId playerId, const common::PlayerState &newState) {
    std::lock_guard<std::mutex> lock(mutex_);
    playerStates_[playerId] = newState;
    lastUpdate_[playerId] = std::chrono::steady_clock::now();
  }

  // Update player position and velocity
  void updatePlayerMovement(common::PlayerId playerId, double x, double y, double vx, double vy, double direction) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playerStates_.find(playerId);
    if (it != playerStates_.end()) {
      it->second.x = x;
      it->second.y = y;
      it->second.vx = vx;
      it->second.vy = vy;
      it->second.direction = direction;
      lastUpdate_[playerId] = std::chrono::steady_clock::now();
    }
  }

  // Update player health
  void updatePlayerHealth(common::PlayerId playerId, int health) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playerStates_.find(playerId);
    if (it != playerStates_.end()) {
      it->second.health = health;
      it->second.isAlive = (health > 0);
      lastUpdate_[playerId] = std::chrono::steady_clock::now();
    }
  }

  // Get player state
  std::optional<common::PlayerState> getPlayerState(common::PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playerStates_.find(playerId);
    if (it != playerStates_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Get all player states in a room
  std::unordered_map<common::PlayerId, common::PlayerState> getRoomPlayerStates(const std::vector<common::PlayerId> &playerIds) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<common::PlayerId, common::PlayerState> states;
    for (common::PlayerId pid : playerIds) {
      auto it = playerStates_.find(pid);
      if (it != playerStates_.end()) {
        states[pid] = it->second;
      }
    }
    return states;
  }

  // Remove player state
  void removePlayer(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    playerStates_.erase(playerId);
    lastUpdate_.erase(playerId);
  }

  // Check if player is alive
  bool isPlayerAlive(common::PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playerStates_.find(playerId);
    return (it != playerStates_.end() && it->second.isAlive);
  }

  // Get last update time
  std::optional<std::chrono::steady_clock::time_point> getLastUpdate(common::PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = lastUpdate_.find(playerId);
    if (it != lastUpdate_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Get all active players
  std::vector<common::PlayerId> getActivePlayers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<common::PlayerId> activePlayers;
    for (const auto &kv : playerStates_) {
      if (kv.second.isAlive) {
        activePlayers.push_back(kv.first);
      }
    }
    return activePlayers;
  }

private:
  mutable std::mutex mutex_;
  std::unordered_map<common::PlayerId, common::PlayerState> playerStates_;
  std::unordered_map<common::PlayerId, std::chrono::steady_clock::time_point> lastUpdate_;
};

} // namespace game
