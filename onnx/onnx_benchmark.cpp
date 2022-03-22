/**
 * You can use
 * g++ -o onnx_benchmark onnx_benchmark.cpp -lonnxruntime -lbenchmark
 * to compile it.
 * Usage:
 * ./onnx_benchmark model [cache_engine] [batch_size]
 * Note that if the cache_engine does not exist, it will be created.
 * So you can also use it to generate cache engines for TensorRT.
 */
#include <assert.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>

#include <benchmark/benchmark.h>

#include "onnxruntime/core/session/onnxruntime_cxx_api.h"

constexpr char kINT8CalibrationTableFile[] = "calibration.flatbuffers";
constexpr const char *kDataTypeIdToTypeName[] = {
	nullptr,   // ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED
	"float32", // ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT
	"uint8",   // ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8
	"int8",	   // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8
	"uint16",  // ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16
	"int16",   // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16
	"int32",   // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32
	"int64",   // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64
	"string",  // ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING
	"bool",	   // ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL
	"float16", // ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16
	"float64", // ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE
	"uint32",  // ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32
	"uint64",  // ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64
	nullptr,   // ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX64
	nullptr,   // ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX128
	nullptr	   // ONNX_TENSOR_ELEMENT_DATA_TYPE_BFLOAT16
};

size_t GetOnnxTypeSize(ONNXTensorElementDataType type)
{
	switch (type)
	{
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: // maps to c type uint8_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8:  // maps to c type int8_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL:
		return 1;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16: // maps to c type uint16_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16:  // maps to c type int16_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16:
		return 2;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT32: // maps to c type uint32_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:  // maps to c type int32_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:  // maps to c type float
		return 4;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT64: // maps to c type uint64_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:  // maps to c type int64_t
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE: // maps to c type double
		return 8;
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_STRING: // maps to c++ type std::string
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED:
	// Non-IEEE floating-point format based on IEEE754  single-precision
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_BFLOAT16:
	// complex with float32 real and imaginary components
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX64:
	// complex with float64 real and imaginary components
	case ONNX_TENSOR_ELEMENT_DATA_TYPE_COMPLEX128:
	default:
		std::cout << "unsupported type" << std::endl;
		return -1;
	}
}

// Initializes input data randomly.
std::vector<int8_t> CreateRandomData(const ONNXTensorElementDataType type,
									 int length)
{
	const size_t size = length * GetOnnxTypeSize(type);
	assert(size > 0);
	std::vector<int8_t> output(size);
	void *data = output.data();
	if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8 ||
		type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8)
	{
		uint8_t *buffer = static_cast<uint8_t *>(data);
		for (int i = 0; i < length; ++i)
		{
			buffer[i] = std::rand() % 255;
		}
	}
	else if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
	{
		float *buffer = static_cast<float *>(data);
		for (int i = 0; i < length; ++i)
		{
			buffer[i] = static_cast<float>(std::rand() % 255) / 255.f;
		}
	}
	else if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL)
	{
		bool *buffer = static_cast<bool *>(data);
		for (int i = 0; i < length; ++i)
		{
			buffer[i] = true;
		}
	}
	else
	{
		memset(data, 0, size);
	}
	return output;
}

Ort::Value CreateRandomInputTensor(const Ort::Session &sess,
								   const Ort::MemoryInfo &memory_info,
								   int input_index, int batch_size = 1)
{
	const Ort::TypeInfo typeinfo = sess.GetInputTypeInfo(input_index);
	const Ort::TensorTypeAndShapeInfo &tensor_info =
		typeinfo.GetTensorTypeAndShapeInfo();
	ONNXTensorElementDataType type = tensor_info.GetElementType();

	const size_t num_dims = tensor_info.GetDimensionsCount();
	std::vector<int64_t> input_node_dims(num_dims);
	tensor_info.GetDimensions(static_cast<int64_t *>(input_node_dims.data()),
							  num_dims);
	size_t total = 1;

	for (size_t i = 0; i < num_dims; i++)
	{
		if (input_node_dims[i] < 0)
		{
			assert(i == 0); // only batch dim is allowed to be dynamic
			input_node_dims[i] = batch_size;
		}
		total *= input_node_dims[i];
	}
	std::vector<int8_t> data = CreateRandomData(type, total);
	return Ort::Value::CreateTensor(memory_info, data.data(), data.size(),
									input_node_dims.data(), num_dims, type);
}

void PrintTypeInfo(const Ort::TensorTypeAndShapeInfo &tensor_info)
{
	const ONNXTensorElementDataType type = tensor_info.GetElementType();
	const size_t num_dims = tensor_info.GetDimensionsCount();
	std::vector<int64_t> dims(num_dims);
	tensor_info.GetDimensions(static_cast<int64_t *>(dims.data()), num_dims);
	std::cout << "(";
	for (size_t i = 0; i < dims.size(); i++)
	{
		int64_t dim = dims[i];
		std::cout << (i == 0 ? "" : ", ") << dim;
	}
	std::cout << ") " << kDataTypeIdToTypeName[type];
}

void PrintTensorsInfo(const std::vector<const char *> &tensor_names,
					  const std::vector<Ort::Value> &tensors)
{
	for (int i = 0; i < tensor_names.size(); i++)
	{
		const std::string name = tensor_names[i];
		std::cout << name << ": ";
		PrintTypeInfo(tensors[i].GetTensorTypeAndShapeInfo());
		std::cout << std::endl;
	}
}

void PrintInputAndOutputInfo(const Ort::Session &sess)
{
	std::cout << "--------------------------------------" << std::endl;
	std::cout << "Model inputs and outputs:" << std::endl;
	const int num_inputs = sess.GetInputCount();
	const int num_outputs = sess.GetOutputCount();
	Ort::AllocatorWithDefaultOptions allocator;
	for (int i = 0; i < num_inputs; i++)
	{
		const Ort::TypeInfo input_info = sess.GetInputTypeInfo(i);
		const char *name =
			sess.GetInputName(i, static_cast<OrtAllocator *>(allocator));
		std::cout << "Input " << i << ": " << name << " ";
		PrintTypeInfo(input_info.GetTensorTypeAndShapeInfo());
		std::cout << std::endl;
	}
	for (int i = 0; i < num_outputs; i++)
	{
		const Ort::TypeInfo output_info = sess.GetOutputTypeInfo(i);
		const char *name =
			sess.GetOutputName(i, static_cast<OrtAllocator *>(allocator));
		std::cout << "Output " << i << ": " << name << " ";
		PrintTypeInfo(output_info.GetTensorTypeAndShapeInfo());
		std::cout << std::endl;
	}
}

void PrintTensorRTEnvs()
{
	for (char **p = environ; *p != nullptr; p++)
	{
		const std::string env_value(*p);
		if (env_value.find("ORT_TENSORRT") == 0)
		{
			std::cout << (env_value) << std::endl;
		}
	}
}

void BM_onnx(benchmark::State &state, const std::string &onnx_file, // NOLINT
			 const std::string &cache_engines_path, int batch_size = 1,
			 int gpu_id = 0)
{
	std::srand(32);
	Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "voy_onnx_benchmark");
	Ort::SessionOptions option;
	option.SetGraphOptimizationLevel(ORT_DISABLE_ALL);
	std::cout << "model path = " << onnx_file << std::endl;
	std::cout << "cache engine path = " << cache_engines_path << std::endl;

	const std::string int8_calibration_table_file =
		cache_engines_path + "/" + kINT8CalibrationTableFile;

	bool int8_enable = false;
	if (std::ifstream(int8_calibration_table_file).good())
	{
		int8_enable = true;
		std::cout << "Using INT8, calibration table: "
				  << int8_calibration_table_file << std::endl;
	}
	else
	{
		std::cout << "Using FP16." << std::endl;
	}

	setenv("ORT_TENSORRT_MAX_PARTITION_ITERATIONS", "1000", 0);
	setenv("ORT_TENSORRT_MIN_SUBGRAPH_SIZE", "1", 0);
	setenv("ORT_TENSORRT_MAX_WORKSPACE_SIZE", "1073741824", 0);
	setenv("ORT_TENSORRT_FP16_ENABLE", "1", 0);
	setenv("ORT_TENSORRT_ENGINE_CACHE_ENABLE", "1", 0);
	setenv("ORT_TENSORRT_CACHE_PATH", cache_engines_path.c_str(), 0);

	if (int8_enable)
	{
		setenv("ORT_TENSORRT_INT8_ENABLE", "1", 1);
		setenv("ORT_TENSORRT_INT8_CALIBRATION_TABLE_NAME",
			   kINT8CalibrationTableFile, 1);
	}

	PrintTensorRTEnvs();
	option.SetExecutionMode(ORT_SEQUENTIAL);
	// option.AppendExecutionProvider_TensorRT({gpu_id});
	option.AppendExecutionProvider_CUDA({});
	option.EnableProfiling("profile");

	std::cout << "start to build session" << std::endl;
	Ort::Session sess(env, onnx_file.c_str(), option);
	std::cout << "session built." << std::endl;

	const size_t num_inputs = sess.GetInputCount();
	const size_t num_outputs = sess.GetOutputCount();
	std::vector<const char *> input_node_names;
	std::vector<const char *> output_node_names;
	std::vector<Ort::Value> input_tensors;
	std::vector<Ort::Value> output_tensors;
	input_tensors.reserve(num_inputs);

	Ort::AllocatorWithDefaultOptions allocator;
	Ort::MemoryInfo memory_info =
		Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeCPUInput);

	PrintInputAndOutputInfo(sess);
	for (size_t i = 0; i < num_inputs; i++)
	{
		input_node_names.push_back(
			sess.GetInputName(i, static_cast<OrtAllocator *>(allocator)));
		input_tensors.emplace_back(
			CreateRandomInputTensor(sess, memory_info, i, batch_size));
	}

	for (int i = 0; i < num_outputs; i++)
	{
		output_node_names.push_back(
			sess.GetOutputName(i, static_cast<OrtAllocator *>(allocator)));
	}

	Ort::RunOptions run_option;
	run_option.SetRunLogVerbosityLevel(5);
	// warmup
	for (int i = 0; i < 3; i++)
	{
		output_tensors =
			sess.Run(run_option, input_node_names.data(), input_tensors.data(),
					 input_node_names.size(), output_node_names.data(),
					 output_node_names.size());
	}

	for (auto _ : state)
	{
		output_tensors =
			sess.Run(run_option, input_node_names.data(), input_tensors.data(),
					 input_node_names.size(), output_node_names.data(),
					 output_node_names.size());
	}

	std::cout << "--------------------------------------" << std::endl;
	std::cout << "Runtime inputs:" << std::endl;
	PrintTensorsInfo(input_node_names, input_tensors);
	std::cout << "Runtime outputs:" << std::endl;
	PrintTensorsInfo(output_node_names, output_tensors);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0]
				  << " model [cache_dir] [batch] [batch2] ..." << std::endl;
		exit(1);
	}
	const std::string model_path = argv[1];

	int left_pos = model_path.find_last_of("/");
	int right_pos = model_path.find_last_of(".");
	std::string filename;
	if (right_pos > left_pos + 1)
	{
		filename = model_path.substr(left_pos + 1, right_pos - left_pos - 1);
	}
	else
	{
		filename = model_path.substr(left_pos + 1);
	}

	if (access(filename.c_str(), 0) != 0)
	{
		mkdir(filename.c_str(), 0777);
	}
	chdir(filename.c_str());

	std::string command = "cp " + model_path + " ./";
	system(command.c_str());

	std::string cache_engine_path = "cache_engine/";
	int batch_size = 1;

	if (argc > 2)
	{
		cache_engine_path = argv[2];
	}

	if (argc > 3)
	{
		batch_size = std::stoi(argv[3]);
		assert(batch_size > 0);
	}

	auto BM_model = [](benchmark::State &state, const std::string &model_path,
					   const std::string &cache_engine_path, int batch_size)
	{
		BM_onnx(state, model_path, cache_engine_path, batch_size);
	};

	benchmark::RegisterBenchmark("onnx", BM_model, model_path, cache_engine_path,
								 batch_size)
		->Iterations(100);

	// when generating cache engine, allow users to generate with different
	// batch sizes
	for (int i = 4; i < argc; i++)
	{
		batch_size = std::stoi(argv[i]);
		assert(batch_size > 1);
		benchmark::RegisterBenchmark("onnx", BM_model, model_path,
									 cache_engine_path, batch_size)
			->Iterations(1);
	}

	argc = 1; // we do not handle benchmark args.
	benchmark::Initialize(&argc, argv);
	benchmark::RunSpecifiedBenchmarks();
}
