#include <xrt/xrt_bo.h>
#include <xrt/xrt_device.h>
#include <xrt/xrt_kernel.h>
#include <xrt/xrt_uuid.h>

#include <vnx/cmac.hpp>
#include <vnx/networklayer.hpp>

/**==========================================================================**/
/**                                   VNX                                    **/
typedef struct vnxIP_ {
  std::string ip_address;
  int port;
  bool valid;
} vnxIP;

constexpr int MAX_SOCKETS = 16;
constexpr int VNX_START_PORT = 5500;

void ConfigureVNX(xrt::device device, int device_id, xrt::uuid uuid,
                  int total_devices, std::unique_ptr<vnx::CMAC> &cmac,
                  std::unique_ptr<vnx::Networklayer> &network_layer) {
  std::vector<vnxIP> ips;
  // Instantiate CMAC kernel
  cmac = std::unique_ptr<vnx::CMAC>(
      new vnx::CMAC(xrt::ip(device, uuid, "cmac_0:{cmac_0}")));
  cmac->set_rs_fec(false);

  // Network kernel
  network_layer = std::unique_ptr<vnx::Networklayer>(new vnx::Networklayer(
      xrt::ip(device, uuid, "networklayer:{networklayer_0}")));

  // See if this sleep is needed
  bool link_status = false;
  for (std::size_t i = 0; i < 5; ++i) {
    auto status = cmac->link_status();
    link_status = status["rx_status"];
    if (link_status) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  if (link_status)
    printf("[vnx %d] Found CMAC link\n", device_id);

  // Create IP Table
  for (int i = 0; i < MAX_SOCKETS; i++) {
    ips.push_back({
        .ip_address = "0.0.0.0",
        .port = 0,
        .valid = 0,
    });
  }

  std::string base_ip = "172.155.10.";
  for (int i = 0; i < total_devices; i++) {
    ips[i].ip_address = base_ip + std::to_string(i + 1);
    ips[i].port = VNX_START_PORT + i;
    ips[i].valid = true;
  }

  // set this FPGA ip
  network_layer->update_ip_address(ips[device_id].ip_address);
  for (int i = 0; i < MAX_SOCKETS; i++) {
    if (i != device_id)
      network_layer->configure_socket(i, ips[i].ip_address, ips[i].port,
                                      ips[device_id].port, ips[i].valid);
  }
  std::map<int, vnx::socket_t> socket_map =
      network_layer->populate_socket_table();

  for (auto &entry : socket_map) {
    printf("[vnx %d] - [%d] %s:%d - %d\n", device_id, entry.first,
           entry.second.theirIP.c_str(), entry.second.theirPort,
           entry.second.valid);
  }

  printf("[vnx %d] Finished configuring vnx on device\n", device_id);
}

void ConfigureARP(std::unique_ptr<vnx::Networklayer> &network_layer) {
  network_layer->arp_discovery();
}

void ShowARPTable(int device_id, int total_devices,
                  std::unique_ptr<vnx::Networklayer> &network_layer) {
  network_layer->arp_discovery();
  auto table = network_layer->read_arp_table(16);
  for (const auto &[id, value] : table) {
    printf("[vnx %d] ARP table: [%d] = %s %s\n", device_id, id,
           value.first.c_str(), value.second.c_str());
  }
}

/**==========================================================================**/
/**                                 Helpers                                  **/
int32_t *create_buffer(int32_t val, int size) {
  int32_t *buf = (int32_t *)aligned_alloc(4096, size * sizeof(int32_t));
  for (int i = 0; i < size; i++)
    buf[i] = val;
  return buf;
}
