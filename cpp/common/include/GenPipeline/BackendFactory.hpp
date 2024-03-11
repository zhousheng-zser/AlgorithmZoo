#ifndef BACKENDFACTORY_H  
#define BACKENDFACTORY_H  
#include <memory>  
#include <string>  
#include <functional>  
#include <unordered_map>
#include <unordered_set>
#include <opencv2/opencv.hpp>
#include <Primitives/tensor.hpp>

//#include "../export/dllexport.hpp"

using namespace glasssix;

class InferBackend {
public:
    enum class SupportManualNormalization { Enable, Disable };

    virtual ~InferBackend() = default;
    virtual SupportManualNormalization manual_possible_normalization(std::array<float, 3> means, std::array<float, 3> stands) = 0;

    virtual int initModel(std::string arch, std::string weight, int device) = 0;
    virtual int initModel(std::string model, int device) = 0;

    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(cv::Mat image) = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(std::shared_ptr<glasssix::memory::tensor<float>> input_tensor) = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const float* input_data, std::vector<int> data_shape, int order) = 0;
    virtual std::unordered_map<std::string, std::shared_ptr<glasssix::memory::tensor<float>>> forward(const std::uint8_t* input_data, std::vector<int> data_shape, int order) = 0;

    virtual const std::string getBackendTypeName() = 0;
    virtual const std::string getVersion() = 0;
};

// BackendFactoryRegistry Singleton
class EXPORT_EXCALIBUR_PRIMITIVES BackendFactoryRegistry {

    using CreatorFunction = std::function<std::unique_ptr<InferBackend>()>;

    bool does_backend_support_model_format_(std::vector<std::string> model_exts, const std::pair<std::string, std::unordered_set<std::string>>& model_format_mapping);

private:
    BackendFactoryRegistry() = default;

public:

    BackendFactoryRegistry(const BackendFactoryRegistry&) = delete;

    BackendFactoryRegistry& operator=(const BackendFactoryRegistry&) = delete;

    static BackendFactoryRegistry& getInstance();

    void registerBackend(const std::string& name, const std::vector<std::string> modelFormats, CreatorFunction factory);

    std::string BackendFormatsHint(const std::vector<std::string>& model_exts);

    std::unique_ptr<InferBackend> createBackend(const std::string& name) const;

    std::unordered_set<std::string> getBackendMenu();

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