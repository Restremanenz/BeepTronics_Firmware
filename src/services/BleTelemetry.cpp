#include "BleTelemetry.h"
#include "Config.h"
#include "core/Lk8ex1.h"
#include <NimBLEDevice.h>
#include <algorithm>

namespace {
constexpr char serviceUuid[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char rxUuid[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr char txUuid[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
class ServerCallbacks : public NimBLEServerCallbacks {
    void onDisconnect(NimBLEServer*) override { NimBLEDevice::startAdvertising(); }
};
ServerCallbacks callbacks;
}

void BleTelemetry::begin() {
    NimBLEDevice::init(config::simulation ? "BeepTronics SIMULATION" : "BeepTronics Vario");
    NimBLEDevice::setMTU(185);
    server_ = NimBLEDevice::createServer();
    server_->setCallbacks(&callbacks, false);
    auto* service = server_->createService(serviceUuid);
    tx_ = service->createCharacteristic(txUuid, NIMBLE_PROPERTY::NOTIFY);
    // Reserved for a future settings protocol; no commands are acted on yet.
    service->createCharacteristic(rxUuid, NIMBLE_PROPERTY::WRITE);
    service->start();
    auto* advertising = server_->getAdvertising();
    advertising->addServiceUUID(serviceUuid);
    advertising->setScanResponse(true);
    advertising->start();
}

void BleTelemetry::update(const Measurement& m, const BatteryReading& battery, uint32_t nowMs) {
    if (!server_ || !m.valid || nowMs - previousMs_ < config::telemetryIntervalMs) return;
    previousMs_ = nowMs;
    if (!server_->getConnectedCount() || !tx_->getSubscribedCount()) return;
    char sentence[96];
    const size_t length = formatLk8ex1(sentence, sizeof(sentence), m, battery);
    // Preserve whole sentences where possible; otherwise fragment the UART byte stream.
    size_t payload = 512;
    for (const auto peer : server_->getPeerDevices()) {
        const uint16_t mtu = server_->getPeerMTU(peer);
        payload = std::min(payload, mtu >= 23 ? size_t(mtu - 3) : size_t(20));
    }
    for (size_t offset = 0; offset < length; offset += payload) {
        tx_->notify(reinterpret_cast<const uint8_t*>(sentence + offset),
                    std::min(payload, length - offset));
    }
}
