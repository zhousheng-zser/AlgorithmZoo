
#include "common.hpp"

//! \brief Enginne
//! 
//! 
class Engine {
public:
	Engine(const Options& options)
		: mOptions(options)
		, mRuntime(nullptr)
		, mEngine(nullptr)
	{
	}

	~Engine();

	bool loadNetwork(std::string modelPath);

	std::shared_ptr<glasssix::memory::tensor <float>> blobFromMats(const std::shared_ptr<glasssix::memory::tensor<float>>& inputImages, const std::array<float, 3>& subVals, const std::array<float, 3>& divVals, bool normalize);
	
	std::unordered_map<std::string, std::vector<float>> runInference(std::shared_ptr<glasssix::memory::tensor<float>>& blob);

private:

	void getDeviceNames(std::vector<std::string>& deviceNames);

	// Input image norm
	std::array<float, 3> mMean{0.f, 0.f, 0.f};
	std::array<float, 3> mVar{1.f, 1.f, 1.f};
	bool mNormalize;
	
	// Options
	const Options mOptions;
	std::string mEngineName;
	Logger mLogger;

	// Buffers
	std::vector<void*> mBuffers;
	std::vector<uint32_t> mOutputLengthsFloat{};
	std::vector<nvinfer1::Dims3> mInputDims{};
	std::vector<nvinfer1::Dims>  mOutputDims{};
	std::vector<std::string> mInputNames{};
	std::vector<std::string> mOutputNames{};
	std::vector<std::string> mIOTensorNames;

	// IRuntime 
	std::shared_ptr<nvinfer1::IRuntime> mRuntime;
	std::shared_ptr<nvinfer1::ICudaEngine> mEngine;
	std::shared_ptr<nvinfer1::IExecutionContext> mContext = nullptr;

};

void Logger::log(Severity severity, const char* msg) noexcept {
    // Only log Warnings or more important.
    if (severity <= Severity::kWARNING) {
        std::cout << msg << std::endl;
    }
}

Engine::~Engine() {
    // Free the GPU memory
    for (auto& buffer : mBuffers) {
        Util::CheckCUDAError(cudaFree(buffer));
    }

    mBuffers.clear();
}

void Engine::getDeviceNames(std::vector<std::string>& deviceNames) {
    int numGPUs;
    cudaGetDeviceCount(&numGPUs);

    for (int device = 0; device < numGPUs; device++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, device);

        deviceNames.push_back(std::string(prop.name));
    }
}

bool Engine::loadNetwork(std::string modelPath) {

    // check modelPath is valid
    if(Util::CheckFileExist(modelPath) == false)
	{
		std::cout << "modelPath is invalid" << std::endl;
		return false;
	}
    
    std::ifstream file(modelPath, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("unable to read engine file");
    }

    mRuntime = std::unique_ptr<nvinfer1::IRuntime>{ nvinfer1::createInferRuntime(mLogger) };

    // set the device index
    auto ret = cudaSetDevice(mOptions.deviceIndex);
    if (ret != 0) {
        int numGPUs;
        cudaGetDeviceCount(&numGPUs);
        auto errMsg = "Unable to set GPU device index to: " + std::to_string(mOptions.deviceIndex) +
            ". Note, your device has " + std::to_string(numGPUs) + " CUDA-capable GPU(s).";
        throw std::runtime_error(errMsg);
    }

    // create the engine
    mEngine = std::unique_ptr<nvinfer1::ICudaEngine>(mRuntime->deserializeCudaEngine(buffer.data(), buffer.size()));
    if (!mEngine) {
		return false;
	}

    // create the execution context
    mContext = std::unique_ptr<nvinfer1::IExecutionContext>(mEngine->createExecutionContext());
    if (!mContext) {
		return false;
	}

    // storage for holding the input and output buffers;
    mBuffers.resize(mEngine->getNbIOTensors());

    // create the CUDA stream used for profiling
    cudaStream_t stream;
    Util::CheckCUDAError(cudaStreamCreate(&stream));

    // Allocate GPU buffers
    mOutputLengthsFloat.clear();

    for (int i = 0; i < mEngine->getNbIOTensors(); ++i) {
        const auto tensorName = mEngine->getIOTensorName(i);
        mIOTensorNames.emplace_back(tensorName);

        const auto tensorType = mEngine->getTensorIOMode(tensorName);
        const auto tensorShape = mEngine->getTensorShape(tensorName);

        if (tensorType == nvinfer1::TensorIOMode::kINPUT)
        {
            // Allocate memory for the input buffer
            Util::CheckCUDAError(cudaMallocAsync(&mBuffers[i], mOptions.maxBatchSize * tensorShape.d[1] * tensorShape.d[2] * tensorShape.d[3] * sizeof(float), stream));

            mInputDims.emplace_back(nvinfer1::Dims3{ tensorShape.d[1], tensorShape.d[2], tensorShape.d[3] });

            mInputNames.push_back(tensorName);

        }
        else if (tensorType == nvinfer1::TensorIOMode::kOUTPUT) {
            // The binding is a output
            uint32_t outputLengthFloat = 1;
            mOutputDims.emplace_back(tensorShape);
            for (int j = 0; j < tensorShape.nbDims; ++j)
            {
                outputLengthFloat *= tensorShape.d[j];
            }

            mOutputLengthsFloat.push_back(outputLengthFloat);

            mOutputNames.push_back(tensorName);

            Util::CheckCUDAError(cudaMallocAsync(&mBuffers[i], outputLengthFloat * mOptions.maxBatchSize * sizeof(float), stream));

        }
        else
        {
            throw std::runtime_error("Error, tensor is not an input or an output!");
        }
    }

    Util::CheckCUDAError(cudaStreamSynchronize(stream));
    Util::CheckCUDAError(cudaStreamDestroy(stream));

    return true;
}

std::shared_ptr<glasssix::memory::tensor<float>> Engine::blobFromMats(const std::shared_ptr<glasssix::memory::tensor<float>>& inputImages, const std::array<float, 3>& subVals, const std::array<float, 3>& divVals, bool normalize)
{

    int c = inputImages->channels();
    int h = inputImages->height();
    int w = inputImages->width();

    if (normalize == true)
    {
		std::shared_ptr<glasssix::memory::tensor <float>> blob(new glasssix::memory::tensor<float>(std::vector<int>{1, c, h, w}, -1, glasssix::memory::NCHW));

		auto inputImagesPtr = inputImages->cpu_data();
		auto blobPtr = blob->mutable_cpu_data();

        for (int i = 0; i < inputImages->count(); ++i)
        {
	        blobPtr[i] = (inputImagesPtr[i] / 255.0f - subVals[i % 3]) / divVals[i % 3];
        }

		return blob;
	}
    else
    {
		std::shared_ptr<glasssix::memory::tensor <float>> blob(new glasssix::memory::tensor<float>(std::vector<int>{1, c, h, w}, -1, glasssix::memory::NCHW));

		auto inputImagesPtr = inputImages->cpu_data();
		auto blobPtr = blob->mutable_cpu_data();

        for (int i = 0; i < inputImages->count(); ++i)
        {
			blobPtr[i] = (inputImagesPtr[i] - subVals[i % 3]) / divVals[i % 3];
		}

		return blob;
	}   
}

std::unordered_map<std::string, std::vector<float>> Engine::runInference(std::shared_ptr<glasssix::memory::tensor<float>>& blob)
{
    std::unordered_map<std::string, std::vector<float>> temp;

    auto dims = mInputDims[0];
    // Create CUDA stream for inference;
    cudaStream_t inferenceCudaStream;
    Util::CheckCUDAError(cudaStreamCreate(&inferenceCudaStream));

    nvinfer1::Dims4 inputDims = { 1, dims.d[0], dims.d[1], dims.d[2]};
    
    mContext->setInputShape(mIOTensorNames[0].c_str(), inputDims);
        
    auto mBlobPtr = blob->cpu_data();

    Util::CheckCUDAError(cudaMemcpyAsync(
		mBuffers[0],
		mBlobPtr,
		blob->count() * sizeof(float),
		cudaMemcpyHostToDevice,
		inferenceCudaStream
	));

    // Ensure all dynamic bindings have been updated
    if(!mContext->allInputDimensionsSpecified()) {
		throw std::runtime_error("Not all input dimensions specified!");
	}

    // Set the address of the input and output buffers;
    mContext->setTensorAddress(mIOTensorNames[0].c_str(), mBuffers[0]);

    mContext->enqueueV3(inferenceCudaStream);

    // mInputDims.size() = 1 
    // mOutputDims.size() = n
    for (int32_t outputBinding = 1; outputBinding < mEngine->getNbBindings(); ++outputBinding)
    {
        // We start at index m_inputDims.size() to account for the inputs in our m_buffers
        std::vector<float> output;
        auto outputLenFloat = mOutputLengthsFloat[outputBinding - 1];
        output.resize(outputLenFloat);
        // Copy the output
        Util::CheckCUDAError(cudaMemcpyAsync(output.data(), static_cast<char*>(mBuffers[outputBinding]) + sizeof(float) * outputLenFloat, outputLenFloat * sizeof(float), cudaMemcpyDeviceToHost, inferenceCudaStream));

        temp.insert(std::make_pair(mOutputNames[outputBinding - 1], output));
    }

    // Synchronize the cuda stream
    Util::CheckCUDAError(cudaStreamSynchronize(inferenceCudaStream));
    Util::CheckCUDAError(cudaStreamDestroy(inferenceCudaStream));
    
    return temp;
}