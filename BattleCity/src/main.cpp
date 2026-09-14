#include <asio.hpp>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
/@

#include "src/network/udp_server.hpp"
#include "src/game/room_manager.hpp"
#include "src/game/player_state_manager.hpp"

int main(int argc, char **argv) {
  try {
    std::uint16_t port = 7777;
    if (argc > 1) {
      const unsigned long p = std::strtoul(argv[1], nullptr, 10);
      if (p == 0 || p > 65535) {
        std::cerr << "Invalid port: " << argv[1] << "\n";
        return 2;
      }
      port = static_cast<std::uint16_t>(p);
    }

    asio::io_context ioContext;

    net::UdpServer server(ioContext, port);
    game::RoomManager roomManager;
    game::PlayerStateManager playerStateManager;
    
    std::cout << "UDP server bound at " << server.localEndpoint() << "\n";
    std::cout << "Room and player management initialized\n";

    // Start receive loop with login and room management
    server.start([&server, &roomManager, &playerStateManager](common::PlayerId pid, const std::string &msg) {
      std::cout << "recv from player " << pid << ": " << msg << "\n";
      
      std::istringstream iss(msg);
      std::string command;
      iss >> command;
      
      if (command == "login") {
        // Player login - initialize state and auto-join room
        playerStateManager.initializePlayer(pid, 100.0, 100.0); // Start at position (100, 100)
        
        // Auto-match to available room or create new one
        auto roomId = roomManager.findAvailableRoom();
        if (!roomId) {
          roomId = roomManager.createRoom();
        }
        
        if (roomManager.joinRoom(*roomId, pid)) {
          std::string response = "login_success pid=" + std::to_string(pid) + " room=" + std::to_string(*roomId);
          server.broadcast(response);
          std::cout << "Player " << pid << " logged in and joined room " << *roomId << "\n";
        } else {
          std::string response = "login_failed room_full";
          server.broadcast(response);
        }
      }
      else if (command == "join_room") {
        // Manual room joining
        common::RoomId roomId;
        if (iss >> roomId) {
          if (roomManager.joinRoom(roomId, pid)) {
            std::string response = "join_success room=" + std::to_string(roomId);
            server.broadcast(response);
            std::cout << "Player " << pid << " joined room " << roomId << "\n";
          } else {
            std::string response = "join_failed room_full_or_invalid";
            server.broadcast(response);
          }
        }
      }
      else if (command == "update") {
        // Update player state (position, velocity, direction, health)
        double x, y, vx, vy, direction;
        int health;
        if (iss >> x >> y >> vx >> vy >> direction >> health) {
          playerStateManager.updatePlayerMovement(pid, x, y, vx, vy, direction);
          playerStateManager.updatePlayerHealth(pid, health);
          
          // Get player's room and broadcast state to room members
          auto roomId = roomManager.getPlayerRoom(pid);
          if (roomId) {
            auto room = roomManager.getRoom(*roomId);
            if (room) {
              auto states = playerStateManager.getRoomPlayerStates(room->players);
              std::string stateMsg = "game_state";
              for (const auto &kv : states) {
                const auto &state = kv.second;
                stateMsg += " p" + std::to_string(kv.first) + 
                           ":" + std::to_string(state.x) + "," + std::to_string(state.y) +
                           "," + std::to_string(state.vx) + "," + std::to_string(state.vy) +
                           "," + std::to_string(state.direction) + "," + std::to_string(state.health);
              }
              server.broadcast(stateMsg);
            }
          }
        }
      }
      else if (command == "list_rooms") {
        // List all active rooms
        auto rooms = roomManager.getActiveRooms();
        std::string response = "rooms_list";
        for (const auto &room : rooms) {
          response += " r" + std::to_string(room.roomId) + 
                     ":" + room.name + ":" + std::to_string(room.players.size()) + "/" + std::to_string(room.maxPlayers);
        }
        server.broadcast(response);
      }
      else {
        // Default broadcast for other messages
        const std::string reply = "[broadcast] pid=" + std::to_string(pid) + ", msg=" + msg;
        server.broadcast(reply);
      }
    });

    auto workGuard = asio::make_work_guard(ioContext);
    asio::signal_set signals(ioContext, SIGINT, SIGTERM);
    signals.async_wait([&](const asio::error_code &, int) {
      std::cout << "Signal caught, shutting down...\n";
      workGuard.reset();
      ioContext.stop();
    });

    ioContext.run();
  } catch (const std::exception &ex) {
    std::cerr << "Fatal error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}

