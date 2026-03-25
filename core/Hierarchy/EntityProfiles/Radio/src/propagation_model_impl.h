#pragma once // compiler pragma
// Concrete propagation model implementation.
// Holds a registry of all radios and their last known positions.
// The math/physics live in src/propagation_model.cpp.
#include "core/Hierarchy/EntityProfiles/Radio/include/radio/radio_interface.h" // include dependency
#include "core/Hierarchy/EntityProfiles/Radio/include/radio/radio_config.h" // include dependency
#include <map> // include dependency
#include <mutex> // include dependency

namespace radio { // namespace scope

class PropagationModelImpl : public PropagationModel { // type definition
public: // public section
    void addRadio(Radiolib* radio, const Position& pos) override; // statement
    void removeRadio(Radiolib* radio) override; // statement
    void updateRadioPosition(Radiolib* radio, const Position& new_pos) override; // statement
    void transmit(Radiolib* sender, const std::vector<std::byte>& data, const RadioConfig& tx_config) override; // statement
    std::vector<ScanHit> radiolibscan(Radiolib* scanner) override; // statement
    // by codex: configure model behavior/features.
    void setConfig(const PropagationModelConfig& config) override; // statement
    // by codex: read current model config.
    PropagationModelConfig getConfig() const override; // statement

private: // private section
    struct RadioEntry { // type definition
        Position pos; // statement
    }; // statement

    double computePathLoss(const Position& a, const Position& b, // statement
                           const RadioConfig& tx_config, // statement
                           const RadioConfig& rx_config) const; // statement
    bool canCommunicate(const Position& sender_pos, const Position& receiver_pos, // statement
                        const RadioConfig& tx_config, // statement
                        const RadioConfig& rx_config) const; // statement

    // All radios registered with this model. Key is the Radiolib* used by the UI.
    // Positions are updated via updateRadioPosition() from the middleware loop.
    std::map<Radiolib*, RadioEntry> radios_; // statement
    mutable std::mutex mutex_; // statement
    // by codex: model-level config toggles.
    PropagationModelConfig config_; // statement
}; // statement

} // namespace radio
