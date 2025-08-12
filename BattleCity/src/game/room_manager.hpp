#pragma once

#include <asio.hpp>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "src/common/types.hpp"
#include "src/network/client_manager.hpp"

namespace game {

struct Room {
  common::RoomId roomId;
  std::string name;
  std::vector<common::PlayerId> players;
  std::chrono::steady_clock::time_point lastActivity;
  bool isActive = true;
  static constexpr size_t maxPlayers = 8;
};

class RoomManager {
public:
  RoomManager() : nextRoomId_(1) {}

  // Create a new room and return room ID
  common::RoomId createRoom(const std::string &name = "") {
    std::lock_guard<std::mutex> lock(mutex_);
    const common::RoomId roomId = nextRoomId_++;
    rooms_[roomId] = Room{
        roomId,
        name.empty() ? "Room " + std::to_string(roomId) : name,
        {},
        std::chrono::steady_clock::now(),
        true
    };
    return roomId;
  }

  // Try to join a player to a room
  bool joinRoom(common::RoomId roomId, common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(roomId);
    if (it == rooms_.end() || !it->second.isActive) {
      return false; // Room doesn't exist or is inactive
    }

    Room &room = it->second;
    if (room.players.size() >= Room::maxPlayers) {
      return false; // Room is full
    }

    // Check if player is already in this room
    for (common::PlayerId pid : room.players) {
      if (pid == playerId) {
        return true; // Already in room
      }
    }

    room.players.push_back(playerId);
    room.lastActivity = std::chrono::steady_clock::now();
    playerToRoom_[playerId] = roomId;
    return true;
  }

  // Remove player from room
  void leaveRoom(common::PlayerId playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto roomIt = playerToRoom_.find(playerId);
    if (roomIt == playerToRoom_.end()) {
      return;
    }

    common::RoomId roomId = roomIt->second;
    auto roomIt2 = rooms_.find(roomId);
    if (roomIt2 != rooms_.end()) {
      auto &players = roomIt2->second.players;
      players.erase(std::remove(players.begin(), players.end(), playerId), players.end());
      roomIt2->second.lastActivity = std::chrono::steady_clock::now();
    }

    playerToRoom_.erase(roomIt);
  }

  // Get room info
  std::optional<Room> getRoom(common::RoomId roomId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = rooms_.find(roomId);
    if (it != rooms_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Get player's current room
  std::optional<common::RoomId> getPlayerRoom(common::PlayerId playerId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = playerToRoom_.find(playerId);
    if (it != playerToRoom_.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  // Get all active rooms
  std::vector<Room> getActiveRooms() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Room> activeRooms;
    for (const auto &kv : rooms_) {
      if (kv.second.isActive) {
        activeRooms.push_back(kv.second);
      }
    }
    return activeRooms;
  }

  // Find a room with available space for auto-matching
  std::optional<common::RoomId> findAvailableRoom() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &kv : rooms_) {
      if (kv.second.isActive && kv.second.players.size() < Room::maxPlayers) {
        return kv.first;
      }
    }
    return std::nullopt;
  }

private:
  mutable std::mutex mutex_;
  std::unordered_map<common::RoomId, Room> rooms_;
  std::unordered_map<common::PlayerId, common::RoomId> playerToRoom_;
  std::atomic<common::RoomId> nextRoomId_;
};

} // namespace game
