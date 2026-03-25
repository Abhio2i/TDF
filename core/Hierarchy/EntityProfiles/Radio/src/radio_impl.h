#pragma once // compiler pragma
// Concrete implementation of the Radiolib interface.
// This class is constructed by createRadiolib()/createRadio() in src/radiolib.cpp and used by
// PropagationModelImpl::transmit() (src/propagation_model.cpp) to deliver data.
#include "core/Hierarchy/EntityProfiles/Radio/include/radio/radio_interface.h" // include dependency
#include "core/Hierarchy/EntityProfiles/Radio/include/radio/radio_config.h" // include dependency
#include <mutex> // include dependency
#include <vector> // include dependency
#include <memory> // include dependency

namespace radio { // namespace scope

class PropagationModelImpl; // forward

class RadioImpl : public Radiolib { // type definition
public: // public section
    RadioImpl() = default; // assign or declare

    void configure(const RadioConfig& config) override; // statement
    RadioConfig getConfiguration() const override; // statement
    void setReceiveCallback(ReceiveCallback cb) override; // statement
    void setReceiveCallbackWithMeta(ReceiveCallbackWithMeta cb) override; // statement
    bool transmit(const std::vector<std::byte>& data) override; // statement
    void setPowerOn(bool on) override; // statement
    bool isPoweredOn() const override; // statement
    void setPropagationModel(PropagationModel* model) override; // statement
    std::vector<ScanHit> radiolibscan() override; // statement
    std::vector<ScanHit> getLastScanResults() const override; // statement
    std::vector<Radiolib*> getCommunicableRadios() const override; // statement
    ReceiveReport getLastReceiveReport() const override; // statement

    // Called by PropagationModelImpl::transmit() to deliver a payload.
    void receive(const std::vector<std::byte>& data, const ReceiveReport& report); // statement

private: // private section
    mutable std::mutex mutex_; // statement
    RadioConfig config_; // statement
    bool powered_on_ = true; // assign or declare
    ReceiveCallback receive_cb_; // statement
    ReceiveCallbackWithMeta receive_cb_with_meta_; // statement
    // Set via setPropagationModel(); non-owning pointer to shared model.
    PropagationModel* prop_model_ = nullptr; // assign or declare
    std::vector<ScanHit> last_scan_results_; // statement
    std::vector<Radiolib*> communicable_radios_; // statement
    ReceiveReport last_receive_report_; // statement
}; // statement

} // namespace radio
