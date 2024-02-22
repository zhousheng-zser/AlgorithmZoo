#ifndef BACKENDFACTORY_H  
#define BACKENDFACTORY_H  
#include <map>
#include <memory>  
#include <string>  
#include <functional>  
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>
#include <Primitives/logger.hpp>
#include <Primitives/abi/exceptions.hpp>

using namespace glasssix;

class InferBackend {
public:
    enum class SupportHandNormaliztion { Enable, Disable };

    virtual ~InferBackend() = default;
    virtual SupportHandNormaliztion handset_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) = 0;

    virtual int initModel(std::string arch, std::string weight, int device) = 0;
    virtual int initModel(std::string model, int device) = 0;

    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_tensor) = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, int order) = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const std::uint8_t* input_data, std::vector<int> data_shape, int order) = 0;

    virtual const std::string getBackendTypeName() = 0;
    virtual const std::string getVersion() = 0;
};


class BackendFactoryRegistry {
    using CreatorFunction = std::function<std::unique_ptr<InferBackend>()>;

    bool does_backend_support_model_format_(std::vector<std::string> model_exts, const std::pair<std::string, std::unordered_set<std::string>>& model_format_mapping) {
        bool is_hint{ false };
        for (auto& model_ext : model_exts) {
			if (*model_ext.begin() != '.') model_ext = '.' + model_ext;
            if (model_format_mapping.second.count(model_ext)) {
                is_hint = true; //is acceptableFormat, match successfully
            }
        }
        return is_hint;
    }

public:
    static BackendFactoryRegistry& getInstance() {
        static BackendFactoryRegistry instance;
        return instance;
    }

    void registerBackend(const std::string& name, const std::vector<std::string> modelFormats, CreatorFunction factory) {
        creators_[name] = std::move(factory);

        std::unordered_set<std::string> modelFormats_with_check;
        for (auto modelFormat : modelFormats) {
            if (*modelFormat.begin() != '.') modelFormat = '.' + modelFormat;
            modelFormats_with_check.insert(modelFormat);
        }
        backend_to_supported_model_formats_[name] = modelFormats_with_check;
    }

    std::string BackendFormatsHint(const std::vector<std::string>& model_exts) {
        size_t hint_counter = 0;
        std::string matchBackend;

        if (backend_to_supported_model_formats_.empty()) {
            throw glasssix::exposing::abi_invalid_argument("The registered Backends is empty.");
        }

        for (const auto& model_format_mapping : backend_to_supported_model_formats_) {
            if (does_backend_support_model_format_(model_exts, model_format_mapping)) {
                hint_counter++;
                if (hint_counter < 2) {
					matchBackend = model_format_mapping.first;
                }
                else {
                    std::string exceptionsInfo = "Bad Backends modelFormats intersection.\
                        Different backends should not have the same acceptance format. Please check beckend " + matchBackend + " and " + model_format_mapping.first;
                    throw glasssix::exposing::abi_invalid_operation(exceptionsInfo.c_str());
                }
            }
        }

        // Check if there exists a matching InferBackend.
        if (hint_counter == 0) {
            throw glasssix::exposing::abi_invalid_argument("No matching backends found for the given model extensions.");
        }

        return matchBackend;
    }

    std::unique_ptr<InferBackend> createBackend(const std::string& name) const {
        auto it = creators_.find(name);
        if (it != creators_.end()) {
            return it->second();
        }
        else {
            throw glasssix::exposing::abi_invalid_argument("Invalid bckend name or empty backend creators_.");
        }
    }

    std::unordered_set<std::string> getBackendMenu() {
        std::unordered_set<std::string> backends_menu;
        for (const auto& crt : creators_) {
            backends_menu.insert(crt.first);
		}
        return backends_menu;
    }

private:
    std::unordered_map<std::string, CreatorFunction> creators_;
    // A mapping from InferBackend type to the set of model formats it supports.  
    std::unordered_map<std::string, std::unordered_set<std::string>> backend_to_supported_model_formats_;
};


template <typename T>
class BackendAutoEnroller {
public:
    BackendAutoEnroller() {
        BackendFactoryRegistry::getInstance().registerBackend(T::backendType, T::modelFormats, []() {
            return std::make_unique<T>();
            });
    }
};

#define REGISTER_BACKEND(BackendClass) \
    static BackendAutoEnroller<BackendClass> registrar##BackendClass


#endif