#include "radio_impl.h" // include dependency
// Implementation of the Radiolib interface (see include/radio/radio_interface.h).
// Cross-file flow:
// - transmit(): calls PropagationModelImpl::transmit() in src/propagation_model.cpp
// - receive(): called by PropagationModelImpl::transmit()
// - radiolibscan(): calls PropagationModelImpl::radiolibscan()
#include "propagation_model_impl.h" // for access to transmit
#include <openssl/evp.h> // include dependency
#include <algorithm> // include dependency
#include <memory> // include dependency
#include <utility> // include dependency

namespace radio { // namespace scope

namespace { // namespace scope

struct CipherSpec { // type definition
    const EVP_CIPHER* cipher = nullptr; // assign or declare
    size_t key_len = 0; // assign or declare
    size_t iv_len = 0; // assign or declare
}; // statement

size_t aes_key_length(const std::vector<std::byte>& key) { // function start
    if (key.size() >= 32) return 32; // condition check
    if (key.size() >= 24) return 24; // condition check
    if (key.size() >= 16) return 16; // condition check
    return 32; // default to AES-256 with zero padding
} // block end

CipherSpec get_cipher_spec(const RadioConfig& config) { // function start
    switch (config.encryption_type) { // switch on value
    case EncryptionType::AES: { // case label
        size_t key_len = aes_key_length(config.encryption_key); // assign or declare
        const EVP_CIPHER* cipher = nullptr; // assign or declare
        if (key_len == 16) cipher = EVP_aes_128_ctr(); // condition check
        else if (key_len == 24) cipher = EVP_aes_192_ctr(); // alternate condition
        else cipher = EVP_aes_256_ctr(); // alternate branch
        return {cipher, key_len, 16}; // return value
    } // block end
    case EncryptionType::DES: // case label
        return {EVP_des_cbc(), 8, 8}; // return value
    case EncryptionType::NONE: // case label
        return {nullptr, 0, 0}; // return value
    } // block end
    return {nullptr, 0, 0}; // return value
} // block end

std::vector<unsigned char> normalize_bytes(const std::vector<std::byte>& data, size_t len) { // function start
    std::vector<unsigned char> out(len, 0); // statement
    size_t copy_len = std::min(len, data.size()); // assign or declare
    for (size_t i = 0; i < copy_len; ++i) { // loop over range
        out[i] = static_cast<unsigned char>(data[i]); // assign or declare
    } // block end
    return out; // return value
} // block end

bool evp_transform(const std::vector<std::byte>& in, // statement
                   std::vector<std::byte>& out, // statement
                   const EVP_CIPHER* cipher, // statement
                   const unsigned char* key, // statement
                   const unsigned char* iv, // statement
                   bool encrypt) { // statement
    if (!cipher) return false; // condition check

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new(); // assign or declare
    if (!ctx) return false; // condition check

    int ok = encrypt // statement
                 ? EVP_EncryptInit_ex(ctx, cipher, nullptr, key, iv) // statement
                 : EVP_DecryptInit_ex(ctx, cipher, nullptr, key, iv); // statement
    if (ok != 1) { // condition check
        EVP_CIPHER_CTX_free(ctx); // statement
        return false; // return value
    } // block end

    out.resize(in.size() + static_cast<size_t>(EVP_CIPHER_block_size(cipher))); // statement

    int out_len1 = 0; // assign or declare
    if (!in.empty()) { // condition check
        ok = encrypt // statement
                 ? EVP_EncryptUpdate(ctx, // statement
                                     reinterpret_cast<unsigned char*>(out.data()), // statement
                                     &out_len1, // statement
                                     reinterpret_cast<const unsigned char*>(in.data()), // statement
                                     static_cast<int>(in.size())) // statement
                 : EVP_DecryptUpdate(ctx, // statement
                                     reinterpret_cast<unsigned char*>(out.data()), // statement
                                     &out_len1, // statement
                                     reinterpret_cast<const unsigned char*>(in.data()), // statement
                                     static_cast<int>(in.size())); // statement
    } else { // statement
        out_len1 = 0; // assign or declare
        ok = 1; // assign or declare
    } // block end

    if (ok != 1) { // condition check
        EVP_CIPHER_CTX_free(ctx); // statement
        return false; // return value
    } // block end

    int out_len2 = 0; // assign or declare
    ok = encrypt // statement
             ? EVP_EncryptFinal_ex(ctx, // statement
                                   reinterpret_cast<unsigned char*>(out.data()) + out_len1, // statement
                                   &out_len2) // statement
             : EVP_DecryptFinal_ex(ctx, // statement
                                   reinterpret_cast<unsigned char*>(out.data()) + out_len1, // statement
                                   &out_len2); // statement

    if (ok != 1) { // condition check
        EVP_CIPHER_CTX_free(ctx); // statement
        return false; // return value
    } // block end

    out.resize(static_cast<size_t>(out_len1 + out_len2)); // statement
    EVP_CIPHER_CTX_free(ctx); // statement
    return true; // return value
} // block end

bool encode_payload(const std::vector<std::byte>& data, // statement
                    const RadioConfig& config, // statement
                    std::vector<std::byte>& out) { // statement
    if (config.encryption_type == EncryptionType::NONE) { // condition check
        out = data; // assign or declare
        return true; // return value
    } // block end

    CipherSpec spec = get_cipher_spec(config); // assign or declare
    if (!spec.cipher) return false; // condition check

    auto key = normalize_bytes(config.encryption_key, spec.key_len); // assign or declare
    auto iv = normalize_bytes(config.encryption_iv, spec.iv_len); // assign or declare
    return evp_transform(data, out, spec.cipher, key.data(), iv.data(), true); // return value
} // block end

bool decode_payload(const std::vector<std::byte>& data, // statement
                    const RadioConfig& config, // statement
                    std::vector<std::byte>& out) { // statement
    if (config.encryption_type == EncryptionType::NONE) { // condition check
        out = data; // assign or declare
        return true; // return value
    } // block end

    CipherSpec spec = get_cipher_spec(config); // assign or declare
    if (!spec.cipher) return false; // condition check

    auto key = normalize_bytes(config.encryption_key, spec.key_len); // assign or declare
    auto iv = normalize_bytes(config.encryption_iv, spec.iv_len); // assign or declare
    return evp_transform(data, out, spec.cipher, key.data(), iv.data(), false); // return value
} // block end

} // namespace

void RadioImpl::configure(const RadioConfig& config) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    config_ = config; // assign or declare
} // block end

RadioConfig RadioImpl::getConfiguration() const { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    return config_; // return value
} // block end

void RadioImpl::setReceiveCallback(ReceiveCallback cb) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    receive_cb_ = std::move(cb); // assign or declare
} // block end

void RadioImpl::setReceiveCallbackWithMeta(ReceiveCallbackWithMeta cb) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    receive_cb_with_meta_ = std::move(cb); // assign or declare
} // block end

// Encode (if enabled) and hand off to the propagation model.
// The model is responsible for link-budget checks and delivering to receivers.
bool RadioImpl::transmit(const std::vector<std::byte>& data) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    if (!powered_on_ || config_.mode == RadioMode::RECEIVER_ONLY) // condition check
        return false; // return value
    if (!prop_model_) return false; // not associated with a model

    // Delegate to propagation model
    if (config_.encryption_type == EncryptionType::NONE) { // condition check
        prop_model_->transmit(this, data, config_); // statement
    } else { // statement
        std::vector<std::byte> encoded; // statement
        if (!encode_payload(data, config_, encoded)) { // condition check
            return false; // return value
        } // block end
        prop_model_->transmit(this, encoded, config_); // statement
    } // block end
    return true; // return value
} // block end

void RadioImpl::setPowerOn(bool on) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    powered_on_ = on; // assign or declare
} // block end

bool RadioImpl::isPoweredOn() const { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    return powered_on_; // return value
} // block end

void RadioImpl::setPropagationModel(PropagationModel* model) { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    prop_model_ = model; // assign or declare
} // block end

// Scan is delegated to the propagation model; results are cached locally.
std::vector<ScanHit> RadioImpl::radiolibscan() { // function start
    PropagationModel* model = nullptr; // assign or declare
    { // block start
        std::lock_guard<std::mutex> lock(mutex_); // statement
        model = prop_model_; // assign or declare
    } // block end
    if (!model) { // condition check
        std::lock_guard<std::mutex> lock(mutex_); // statement
        last_scan_results_.clear(); // statement
        communicable_radios_.clear(); // statement
        return last_scan_results_; // return value
    } // block end

    auto results = model->radiolibscan(this); // assign or declare
    { // block start
        std::lock_guard<std::mutex> lock(mutex_); // statement
        last_scan_results_ = results; // assign or declare
        communicable_radios_.clear(); // statement
        communicable_radios_.reserve(results.size()); // statement
        for (const auto& hit : results) { // loop over range
            if (hit.radio) { // condition check
                communicable_radios_.push_back(hit.radio); // statement
            } // block end
        } // block end
    } // block end
    return results; // return value
} // block end

std::vector<ScanHit> RadioImpl::getLastScanResults() const { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    return last_scan_results_; // return value
} // block end

std::vector<Radiolib*> RadioImpl::getCommunicableRadios() const { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    return communicable_radios_; // return value
} // block end

ReceiveReport RadioImpl::getLastReceiveReport() const { // function start
    std::lock_guard<std::mutex> lock(mutex_); // statement
    return last_receive_report_; // return value
} // block end

// Called only by PropagationModelImpl::transmit() when delivery succeeds.
// Decodes (if enabled) and invokes user callbacks.
void RadioImpl::receive(const std::vector<std::byte>& data, const ReceiveReport& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!powered_on_ || config_.mode == RadioMode::TRANSMITTER_ONLY)
        return;

    const std::vector<std::byte>* payload = &data;
    std::vector<std::byte> decoded;
    if (config_.encryption_type != EncryptionType::NONE) {
        if (!decode_payload(data, config_, decoded)) {
            return;
        }
        payload = &decoded;
    }

    last_receive_report_ = report;
    if (receive_cb_) {
        receive_cb_(*payload);
    }
    if (receive_cb_with_meta_) {
        receive_cb_with_meta_(*payload, report);
    }
}

// Factory: constructs the concrete RadioImpl but returns the abstract Radiolib API.
std::unique_ptr<Radiolib> createRadiolib() { // function start
    return std::make_unique<RadioImpl>(); // return value
} // block end

std::unique_ptr<Radiolib> createRadio() { // function start
    return createRadiolib(); // return value
} // block end

Radiolib* createRadiolibRaw() { // function start
    return new RadioImpl(); // return value
} // block end

void destroyRadiolib(Radiolib* radio) { // function start
    delete radio; // statement
} // block end

// Convenience helper: build a model + radio, configure, and attach.
RadioSystem createRadioSystem(const RadioConfig& radio_config, // statement
                              const Position& pos, // statement
                              const PropagationModelConfig& model_config) { // statement
    RadioSystem system; // statement
    system.model = createPropagationModel(model_config); // assign or declare
    system.radio = createRadiolib(); // assign or declare
    attachRadioToModel(system.radio.get(), system.model.get(), radio_config, pos); // statement
    return system; // return value
} // block end

// Wires a Radiolib to a model and registers the initial position.
// This is the common setup used by middleware.
void attachRadioToModel(Radiolib* radio, // statement
                        PropagationModel* model, // statement
                        const RadioConfig& radio_config, // statement
                        const Position& pos) { // statement
    if (!radio || !model) return; // condition check
    radio->setPropagationModel(model); // statement
    radio->configure(radio_config); // statement
    model->addRadio(radio, pos); // statement
} // block end

} // namespace radio
