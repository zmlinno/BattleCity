#pragma once

#include <cstdint>

namespace common {

using PlayerId = std::uint64_t;
using RoomId = std::uint32_t;

// Player state data structure
struct PlayerState {
  double x = 0.0;        // position x
  double y = 0.0;        // position y
  double vx = 0.0;       // velocity x
  double vy = 0.0;       // velocity y
  double direction = 0.0; // facing direction (radians)
  int health = 100;      // health points
  bool isAlive = true;   // alive status
};

// Message types for client-server communication
enum class MessageType : std::uint8_t {
  LOGIN = 1,
  LOGIN_RESPONSE = 2,
  JOIN_ROOM = 3,
  JOIN_ROOM_RESPONSE = 4,
  PLAYER_UPDATE = 5,
  GAME_STATE = 6,
  BROADCAST = 7
};

} // namespace common

