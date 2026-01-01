#ifdef USE_ESP_IDF

#include "ddp.h"
#include "esphome/core/log.h"

#include <lwip/sockets.h>
#include <lwip/inet.h>

#include <array>
#include <cstdio>
#include <cerrno>

namespace esphome {
namespace ddp {

static const char *const TAG = "ddp";
static const int PORT = 4048;

void DDPComponent::loop() {
  if (this->socket_ == nullptr || !this->socket_->ready()) { return; }

  int fd = this->socket_->get_fd();
  if (fd < 0) {
    return;
  }

  static constexpr size_t kMaxPacketSize = 1500;
  std::array<uint8_t, kMaxPacketSize> payload{};

  while (this->socket_ != nullptr && this->socket_->ready()) {
    struct sockaddr_storage client_addr = {};
    socklen_t addr_len = sizeof(client_addr);
    ssize_t len =
        recvfrom(fd, payload.data(), payload.size(), MSG_DONTWAIT, (struct sockaddr *) &client_addr, &addr_len);
    if (len < 0) {
      if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        ESP_LOGW(TAG, "recvfrom failed: %d", errno);
      }
      return;
    }
    if (len == 0) {
      return;
    }

    if (!this->process_(payload.data(), static_cast<uint16_t>(len))) {
      continue;
    }

    if (this->stats_interval_ms_ != 0) {
      char source[64];
      source[0] = '\0';
      if (client_addr.ss_family == AF_INET) {
        auto *addr = reinterpret_cast<struct sockaddr_in *>(&client_addr);
        const char *ip = inet_ntoa(addr->sin_addr);
        uint16_t port = ntohs(addr->sin_port);
        if (ip != nullptr) {
          snprintf(source, sizeof(source), "%s:%u", ip, port);
        }
      }
      this->note_packet_(source, static_cast<uint16_t>(len));
    }
  }
}

void DDPComponent::add_effect(DDPLightEffectBase *light_effect) {
  if (light_effects_.count(light_effect)) {
    return;
  }

  // only the first effect added needs to start udp listening
  // but we still need to add the effect to the set so it can be applied.
  if (this->light_effects_.empty()) {
    if (this->socket_ == nullptr) {
      this->socket_ = socket::socket_ip_loop_monitored(SOCK_DGRAM, IPPROTO_UDP);
    }
    if (this->socket_ == nullptr) {
      ESP_LOGE(TAG, "Socket create failed");
      mark_failed();
      return;
    }

    int enable = 1;
    this->socket_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    struct sockaddr_storage server_addr = {};
    socklen_t addr_len =
        socket::set_sockaddr_any((struct sockaddr *) &server_addr, sizeof(server_addr), PORT);

    int err = this->socket_->bind((struct sockaddr *) &server_addr, addr_len);
    if (err != 0) {
      ESP_LOGE(TAG, "Cannot bind DDP to port %d (err=%d).", PORT, errno);
      this->socket_ = nullptr;
      mark_failed();
      return;
    }

    ESP_LOGD(TAG, "Starting UDP listening for DDP.");
  }

  this->light_effects_.insert(light_effect);
}

void DDPComponent::remove_effect(DDPLightEffectBase *light_effect) {
  if (!this->light_effects_.count(light_effect)) {
    return;
  }

  this->light_effects_.erase(light_effect);

  // if no more effects left, stop udp listening
  if (this->light_effects_.empty() && this->socket_ != nullptr) {
    ESP_LOGD(TAG, "Stopping UDP listening for DDP.");
    this->socket_->close();
    this->socket_ = nullptr;
  }
}

}  // namespace ddp
}  // namespace esphome

#endif  // USE_ESP_IDF
