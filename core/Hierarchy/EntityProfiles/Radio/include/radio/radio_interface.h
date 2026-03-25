#pragma once
// RadioLib public API boundary.
// This header exposes only abstract interfaces and factory helpers.
// Concrete implementations live in:
// - src/radio_impl.h / src/radiolib.cpp (RadioImpl)
// - src/propagation_model_impl.h / src/propagation_model.cpp (PropagationModelImpl)
//
// Cross-file flow (high level):
// - createRadiolib() / createRadio() -> constructs RadioImpl in src/radiolib.cpp
// - createPropagationModel() -> constructs PropagationModelImpl in src/propagation_model.cpp
// - attachRadioToModel() -> wires Radiolib to model and registers its initial position
// - Radiolib::transmit() -> PropagationModel::transmit() -> RadioImpl::receive()
// - Radiolib::radiolibscan() -> PropagationModel::radiolibscan()
#include "radio_config.h"
// by codex: propagation model configuration
#include "propagation_model_config.h"
#include <memory>
#include <vector>
#include <functional>
#include <cstddef>

namespace radio {

// Forward declarations
class PropagationModel;

// Represents a single radio instance.
// Implemented by RadioImpl (src/radio_impl.h), created via createRadiolib()/createRadio().
class Radiolib {
public:
    virtual ~Radiolib() = default;

    // Configure the radio (stores RadioConfig in RadioImpl).
    virtual void configure(const RadioConfig& config) = 0;

    // Get current configuration (from RadioImpl).
    virtual RadioConfig getConfiguration() const = 0;

    // Set callback for received data.
    using ReceiveCallback = std::function<void(const std::vector<std::byte>& data)>;
    virtual void setReceiveCallback(ReceiveCallback cb) = 0;
    // Set callback for received data with signal metadata.
    using ReceiveCallbackWithMeta = std::function<void(const std::vector<std::byte>& data,
                                                       const ReceiveReport& report)>;
    virtual void setReceiveCallbackWithMeta(ReceiveCallbackWithMeta cb) = 0;

    // Transmit data. Returns true if transmission was initiated.
    // This calls PropagationModel::transmit() on the attached model.
    virtual bool transmit(const std::vector<std::byte>& data) = 0;

    // Power control.
    virtual void setPowerOn(bool on) = 0;
    virtual bool isPoweredOn() const = 0;

    // Associate with a propagation model (must be called before first transmit/position update).
    // Typically done via attachRadioToModel().
    virtual void setPropagationModel(PropagationModel* model) = 0;

    // Scan for radios this radio can communicate with (stores results internally).
    // Delegates to PropagationModel::radiolibscan() on the attached model.
    virtual std::vector<ScanHit> radiolibscan() = 0;
    // Get last scan results (cached in RadioImpl).
    virtual std::vector<ScanHit> getLastScanResults() const = 0;
    // Get last scan radio pointers (communicable radios).
    virtual std::vector<Radiolib*> getCommunicableRadios() const = 0;
    // Get last receive report (signal metadata), updated in RadioImpl::receive().
    virtual ReceiveReport getLastReceiveReport() const = 0;
};

// Factory functions to create a radio instance.
// `createRadiolib()` is the preferred typed name.
// `createRadio()` is kept for backward compatibility.
std::unique_ptr<Radiolib> createRadiolib();
std::unique_ptr<Radiolib> createRadio();

// Raw-pointer creation helpers for middleware that stores nullable pointers directly.
Radiolib* createRadiolibRaw();
void destroyRadiolib(Radiolib* radio);

// Propagation model – manages all radios and computes connectivity.
// Implemented by PropagationModelImpl (src/propagation_model_impl.h).
class PropagationModel {
public:
    virtual ~PropagationModel() = default;

    // Add a radio at a given position (stored in PropagationModelImpl::radios_ map).
    virtual void addRadio(Radiolib* radio, const Position& pos) = 0;

    // Remove a radio.
    virtual void removeRadio(Radiolib* radio) = 0;

    // Update a radio's position (kept in the model's radios_ map).
    virtual void updateRadioPosition(Radiolib* radio, const Position& new_pos) = 0;

    // Called internally by radio when transmitting.
    // This is public because Radiolib needs to call it, but it's not for direct user use.
    virtual void transmit(Radiolib* sender, const std::vector<std::byte>& data, const RadioConfig& tx_config) = 0;

    // Scan for radios that the given radio can communicate with.
    virtual std::vector<ScanHit> radiolibscan(Radiolib* scanner) = 0;

    // by codex: configure model behavior/features.
    virtual void setConfig(const PropagationModelConfig& config) = 0;
    // by codex: read current model config.
    virtual PropagationModelConfig getConfig() const = 0;
};

// Factory function to create a propagation model.
std::unique_ptr<PropagationModel> createPropagationModel();
// by codex: factory overload to create a propagation model with explicit config.
std::unique_ptr<PropagationModel> createPropagationModel(const PropagationModelConfig& config);

// Raw-pointer creation helpers for middleware that stores nullable pointers directly.
PropagationModel* createPropagationModelRaw();
PropagationModel* createPropagationModelRaw(const PropagationModelConfig& config);
void destroyPropagationModel(PropagationModel* model);

// Convenience helpers.
struct RadioSystem {
    std::unique_ptr<Radiolib> radio;
    std::unique_ptr<PropagationModel> model;
};

// Create a radio + model, configure, and add the radio to the model at a position.
RadioSystem createRadioSystem(const RadioConfig& radio_config,
                              const Position& pos,
                              const PropagationModelConfig& model_config = {});

// Attach an existing radio to an existing model with config + position.
void attachRadioToModel(Radiolib* radio,
                        PropagationModel* model,
                        const RadioConfig& radio_config,
                        const Position& pos);

} // namespace radio
