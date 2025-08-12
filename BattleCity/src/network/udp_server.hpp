#pragma once

#include <asio.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/network/client_manager.hpp"

namespace net {

class UdpServer {
public:
  using ReceiveCallback = std::function<void(common::PlayerId, const std::string &msg)>;

  UdpServer(asio::io_context &ioContext, std::uint16_t listenPort)
      : ioContext_(ioContext), socket_(ioContext), recvBuffer_(maxDatagramSize) {
    bindSocket(listenPort);
  }

  bool isOpen() const { return socket_.is_open(); }

  asio::ip::udp::endpoint localEndpoint() const { return socket_.local_endpoint(); }

  asio::ip::udp::socket &socket() { return socket_; }

  void start(ReceiveCallback onMessage) {
    onMessage_ = std::move(onMessage);
    postReceive();
  }

  // Broadcast a text message to all connected clients.
  void broadcast(const std::string &message) {
    auto endpoints = clients_.listEndpoints();
    for (const auto &ep : endpoints) {
      socket_.async_send_to(asio::buffer(message), ep, [](auto, auto) {});
    }
  }

private:
  void bindSocket(std::uint16_t listenPort) {
    const asio::ip::udp::endpoint endpoint(asio::ip::udp::v4(), listenPort);

    asio::error_code error;

    socket_.open(endpoint.protocol(), error);
    if (error) {
      throw std::runtime_error("Failed to open UDP socket: " + error.message());
    }

    socket_.set_option(asio::socket_base::reuse_address(true), error);
    if (error) {
      socket_.close();
      throw std::runtime_error("Failed to set SO_REUSEADDR: " + error.message());
    }

    socket_.bind(endpoint, error);
    if (error) {
      socket_.close();
      throw std::runtime_error("Failed to bind UDP socket: " + error.message());
    }
  }

  void postReceive() {
    remoteEndpoint_ = asio::ip::udp::endpoint{};
    socket_.async_receive_from(
        asio::buffer(recvBuffer_), remoteEndpoint_,
        [this](const asio::error_code &ec, std::size_t bytes) {
          if (!ec && bytes > 0) {
            common::PlayerId pid = clients_.getOrCreatePlayerId(remoteEndpoint_);
            if (onMessage_) {
              onMessage_(pid, std::string(recvBuffer_.data(), bytes));
            }
          }
          // Keep receiving regardless of errors to ensure robustness
          postReceive();
        });
  }

  static constexpr std::size_t maxDatagramSize = 1400; // safe MTU payload

  asio::io_context &ioContext_;
  asio::ip::udp::socket socket_;
  net::ClientManager clients_;
  std::vector<char> recvBuffer_;
  asio::ip::udp::endpoint remoteEndpoint_;
  ReceiveCallback onMessage_;
};

} // namespace net

