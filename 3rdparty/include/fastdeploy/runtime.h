// Copyright (c) 2022 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*! \file runtime.h
    \brief A brief file description.

    More details
 */

#pragma once

#include <algorithm>
#include <map>
#include <vector>

#include "fastdeploy/backends/backend.h"
#include "fastdeploy/utils/perf.h"
#include "backends/rknpu/rknpu2/rknpu2_config.h"

/** \brief All C++ FastDeploy APIs are defined inside this namespace
*
*/
namespace fastdeploy {

/*! Inference backend supported in FastDeploy */
enum Backend {
  UNKNOWN,   ///< Unknown inference backend
  ORT,     ///< ONNX Runtime, support Paddle/ONNX format model, CPU / Nvidia GPU
  TRT,      ///< TensorRT, support Paddle/ONNX format model, Nvidia GPU only
  PDINFER,  ///< Paddle Inference, support Paddle format model, CPU / Nvidia GPU
  POROS,    ///< Poros, support TorchScript format model, CPU / Nvidia GPU
  OPENVINO,  ///< Intel OpenVINO, support Paddle/ONNX format, CPU only
  LITE,     ///< Paddle Lite, support Paddle format model, ARM CPU only
  RKNPU2,   ///< RKNPU2, support RKNN format model, Rockchip NPU only
};

FASTDEPLOY_DECL std::ostream& operator<<(std::ostream& out,
                                         const Backend& backend);

/*! Paddle Lite power mode for mobile device. */
enum LitePowerMode {
  LITE_POWER_HIGH = 0,       ///< Use Lite Backend with high power mode
  LITE_POWER_LOW = 1,        ///< Use Lite Backend with low power mode
  LITE_POWER_FULL = 2,       ///< Use Lite Backend with full power mode
  LITE_POWER_NO_BIND = 3,    ///< Use Lite Backend with no bind power mode
  LITE_POWER_RAND_HIGH = 4,  ///< Use Lite Backend with rand high mode
  LITE_POWER_RAND_LOW = 5    ///< Use Lite Backend with rand low power mode
};

FASTDEPLOY_DECL std::string Str(const Backend& b);
FASTDEPLOY_DECL std::string Str(const ModelFormat& f);

/**
 * @brief Get all the available inference backend in FastDeploy
 */
FASTDEPLOY_DECL std::vector<Backend> GetAvailableBackends();

/**
 * @brief Check if the inference backend available
 */
FASTDEPLOY_DECL bool IsBackendAvailable(const Backend& backend);

bool CheckModelFormat(const std::string& model_file,
                      const ModelFormat& model_format);
ModelFormat GuessModelFormat(const std::string& model_file);

/*! @brief Option object used when create a new Runtime object
 */
struct FASTDEPLOY_DECL RuntimeOption {
  /** \brief Set path of model file and parameter file
   *
   * \param[in] model_path Path of model file, e.g ResNet50/model.pdmodel for Paddle format model / ResNet50/model.onnx for ONNX format model
   * \param[in] params_path Path of parameter file, this only used when the model format is Paddle, e.g Resnet50/model.pdiparams
   * \param[in] format Format of the loaded model
   */
  void SetModelPath(const std::string& model_path,
                    const std::string& params_path = "",
                    const ModelFormat& format = ModelFormat::PADDLE);

  /// Use cpu to inference, the runtime will inference on CPU by default
  void UseCpu();

  /// Use Nvidia GPU to inference
  void UseGpu(int gpu_id = 0);

  void UseRKNPU2(fastdeploy::rknpu2::CpuName rknpu2_name
                             = fastdeploy::rknpu2::CpuName::RK3588,
                 fastdeploy::rknpu2::CoreMask rknpu2_core
                             = fastdeploy::rknpu2::CoreMask::RKNN_NPU_CORE_0);

  /// Use TimVX to inference
  void UseTimVX();

  void SetExternalStream(void* external_stream);

  /*
   * @brief Set number of cpu threads while inference on CPU, by default it will decided by the different backends
   */
  void SetCpuThreadNum(int thread_num);

  /// Set ORT graph opt level, default is decide by ONNX Runtime itself
  void SetOrtGraphOptLevel(int level = -1);

  /// Set Paddle Inference as inference backend, support CPU/GPU
  void UsePaddleBackend();

  /// Wrapper function of UsePaddleBackend()
  void UsePaddleInferBackend() {
    return UsePaddleBackend();
  }

  /// Set ONNX Runtime as inference backend, support CPU/GPU
  void UseOrtBackend();

  /// Set TensorRT as inference backend, only support GPU
  void UseTrtBackend();

  /// Set Poros backend as inference backend, support CPU/GPU
  void UsePorosBackend();

  /// Set OpenVINO as inference backend, only support CPU
  void UseOpenVINOBackend();

  /// Set Paddle Lite as inference backend, only support arm cpu
  void UseLiteBackend();

  /// Wrapper function of UseLiteBackend()
  void UsePaddleLiteBackend() {
    return UseLiteBackend();
  }

  /// Set mkldnn switch while using Paddle Inference as inference backend
  void SetPaddleMKLDNN(bool pd_mkldnn = true);

  /*
   * @brief If TensorRT backend is used, EnablePaddleToTrt will change to use Paddle Inference backend, and use its integrated TensorRT instead.
   */
  void EnablePaddleToTrt();

  /**
   * @brief Delete pass by name while using Paddle Inference as inference backend, this can be called multiple times to delete a set of passes
   */
  void DeletePaddleBackendPass(const std::string& delete_pass_name);

  /**
   * @brief Enable print debug information while using Paddle Inference as inference backend, the backend disable the debug information by default
   */
  void EnablePaddleLogInfo();

  /**
   * @brief Disable print debug information while using Paddle Inference as inference backend
   */
  void DisablePaddleLogInfo();

  /**
   * @brief Set shape cache size while using Paddle Inference with mkldnn, by default it will cache all the difference shape
   */
  void SetPaddleMKLDNNCacheSize(int size);

  /**
   * @brief Set device name for OpenVINO, default 'CPU', can also be 'AUTO', 'GPU', 'GPU.1'....
   */
  void SetOpenVINODevice(const std::string& name = "CPU");

  /**
   * @brief Set shape info for OpenVINO
   */
  void SetOpenVINOShapeInfo(
    const std::map<std::string, std::vector<int64_t>>& shape_info) {
    ov_shape_infos = shape_info;
  }

  /**
   * @brief While use OpenVINO backend with intel GPU, use this interface to specify operators run on CPU
   */
  void SetOpenVINOCpuOperators(const std::vector<std::string>& operators) {
    ov_cpu_operators = operators;
  }

  /**
   * @brief Set optimzed model dir for Paddle Lite backend.
   */
  void SetLiteOptimizedModelDir(const std::string& optimized_model_dir);

  /**
   * @brief Set nnadapter subgraph partition path for Paddle Lite backend.
   */
  void SetLiteSubgraphPartitionPath(
    const std::string& nnadapter_subgraph_partition_config_path);

  /**
   * @brief enable half precision while use paddle lite backend
   */
  void EnableLiteFP16();

  /**
   * @brief disable half precision, change to full precision(float32)
   */
  void DisableLiteFP16();

  /**
    * @brief enable int8 precision while use paddle lite backend
    */
  void EnableLiteInt8();

  /**
    * @brief disable int8 precision, change to full precision(float32)
    */
  void DisableLiteInt8();

  /**
   * @brief Set power mode while using Paddle Lite as inference backend, mode(0: LITE_POWER_HIGH; 1: LITE_POWER_LOW; 2: LITE_POWER_FULL; 3: LITE_POWER_NO_BIND, 4: LITE_POWER_RAND_HIGH; 5: LITE_POWER_RAND_LOW, refer [paddle lite](https://paddle-lite.readthedocs.io/zh/latest/api_reference/cxx_api_doc.html#set-power-mode) for more details)
   */
  void SetLitePowerMode(LitePowerMode mode);

  /** \brief Set shape range of input tensor for the model that contain dynamic input shape while using TensorRT backend
   *
   * \param[in] input_name The name of input for the model which is dynamic shape
   * \param[in] min_shape The minimal shape for the input tensor
   * \param[in] opt_shape The optimized shape for the input tensor, just set the most common shape, if set as default value, it will keep same with min_shape
   * \param[in] max_shape The maximum shape for the input tensor, if set as default value, it will keep same with min_shape
   */
  void SetTrtInputShape(
      const std::string& input_name, const std::vector<int32_t>& min_shape,
      const std::vector<int32_t>& opt_shape = std::vector<int32_t>(),
      const std::vector<int32_t>& max_shape = std::vector<int32_t>());

  /// Set max_workspace_size for TensorRT, default 1<<30
  void SetTrtMaxWorkspaceSize(size_t trt_max_workspace_size);

  /// Set max_batch_size for TensorRT, default 32
  void SetTrtMaxBatchSize(size_t max_batch_size);

  /**
   * @brief Enable FP16 inference while using TensorRT backend. Notice: not all the GPU device support FP16, on those device doesn't support FP16, FastDeploy will fallback to FP32 automaticly
   */
  void EnableTrtFP16();

  /// Disable FP16 inference while using TensorRT backend
  void DisableTrtFP16();

  /**
   * @brief Set cache file path while use TensorRT backend. Loadding a Paddle/ONNX model and initialize TensorRT will take a long time, by this interface it will save the tensorrt engine to `cache_file_path`, and load it directly while execute the code again
   */
  void SetTrtCacheFile(const std::string& cache_file_path);

  /**
   * @brief Enable pinned memory. Pinned memory can be utilized to speedup the data transfer between CPU and GPU. Currently it's only suppurted in TRT backend and Paddle Inference backend.
   */
  void EnablePinnedMemory();

  /**
   * @brief Disable pinned memory
   */
  void DisablePinnedMemory();

  /**
   * @brief Enable to collect shape in paddle trt backend
   */
  void EnablePaddleTrtCollectShape();

  /**
   * @brief Disable to collect shape in paddle trt backend
   */
  void DisablePaddleTrtCollectShape();

  /*
   * @brief Set number of streams by the OpenVINO backends
   */
  void SetOpenVINOStreams(int num_streams);

  /** \Use Graphcore IPU to inference.
   *
   * \param[in] device_num the number of IPUs.
   * \param[in] micro_batch_size the batch size in the graph, only work when graph has no batch shape info.
   * \param[in] enable_pipelining enable pipelining.
   * \param[in] batches_per_step the number of batches per run in pipelining.
   */
  void UseIpu(int device_num = 1, int micro_batch_size = 1,
              bool enable_pipelining = false, int batches_per_step = 1);

  /** \brief Set IPU config.
   *
   * \param[in] enable_fp16 enable fp16.
   * \param[in] replica_num the number of graph replication.
   * \param[in] available_memory_proportion the available memory proportion for matmul/conv.
   * \param[in] enable_half_partial enable fp16 partial for matmul, only work with fp16.
   */
  void SetIpuConfig(bool enable_fp16 = false, int replica_num = 1,
                    float available_memory_proportion = 1.0,
                    bool enable_half_partial = false);

  Backend backend = Backend::UNKNOWN;
  // for cpu inference and preprocess
  // default will let the backend choose their own default value
  int cpu_thread_num = -1;
  int device_id = 0;

  Device device = Device::CPU;

  void* external_stream_ = nullptr;

  bool enable_pinned_memory = false;

  // ======Only for ORT Backend========
  // -1 means use default value by ort
  // 0: ORT_DISABLE_ALL 1: ORT_ENABLE_BASIC 2: ORT_ENABLE_EXTENDED 3:
  // ORT_ENABLE_ALL
  int ort_graph_opt_level = -1;
  int ort_inter_op_num_threads = -1;
  // 0: ORT_SEQUENTIAL 1: ORT_PARALLEL
  int ort_execution_mode = -1;

  // ======Only for Paddle Backend=====
  bool pd_enable_mkldnn = true;
  bool pd_enable_log_info = false;
  bool pd_enable_trt = false;
  bool pd_collect_shape = false;
  int pd_mkldnn_cache_size = 1;
  std::vector<std::string> pd_delete_pass_names;

  // ======Only for Paddle IPU Backend =======
  int ipu_device_num = 1;
  int ipu_micro_batch_size = 1;
  bool ipu_enable_pipelining = false;
  int ipu_batches_per_step = 1;
  bool ipu_enable_fp16 = false;
  int ipu_replica_num = 1;
  float ipu_available_memory_proportion = 1.0;
  bool ipu_enable_half_partial = false;

  // ======Only for Paddle-Lite Backend=====
  // 0: LITE_POWER_HIGH 1: LITE_POWER_LOW 2: LITE_POWER_FULL
  // 3: LITE_POWER_NO_BIND 4: LITE_POWER_RAND_HIGH
  // 5: LITE_POWER_RAND_LOW
  LitePowerMode lite_power_mode = LitePowerMode::LITE_POWER_NO_BIND;
  // enable int8 or not
  bool lite_enable_int8 = false;
  // enable fp16 or not
  bool lite_enable_fp16 = false;
  // optimized model dir for CxxConfig
  std::string lite_optimized_model_dir = "";
  std::string lite_nnadapter_subgraph_partition_config_path = "";
  bool enable_timvx = false;

  // ======Only for Trt Backend=======
  std::map<std::string, std::vector<int32_t>> trt_max_shape;
  std::map<std::string, std::vector<int32_t>> trt_min_shape;
  std::map<std::string, std::vector<int32_t>> trt_opt_shape;
  std::string trt_serialize_file = "";
  bool trt_enable_fp16 = false;
  bool trt_enable_int8 = false;
  size_t trt_max_batch_size = 32;
  size_t trt_max_workspace_size = 1 << 30;

  // ======Only for Poros Backend=======
  bool is_dynamic = false;
  bool long_to_int = true;
  bool use_nvidia_tf32 = false;
  int unconst_ops_thres = -1;
  std::string poros_file = "";

  // ======Only for OpenVINO Backend=======
  int ov_num_streams = 0;
  std::string openvino_device = "CPU";
  std::map<std::string, std::vector<int64_t>> ov_shape_infos;
  std::vector<std::string> ov_cpu_operators;

  // ======Only for RKNPU2 Backend=======
  fastdeploy::rknpu2::CpuName rknpu2_cpu_name_
            = fastdeploy::rknpu2::CpuName::RK3588;
  fastdeploy::rknpu2::CoreMask rknpu2_core_mask_
            = fastdeploy::rknpu2::CoreMask::RKNN_NPU_CORE_AUTO;

  std::string model_file = "";  // Path of model file
  std::string params_file = "";  // Path of parameters file, can be empty
  // format of input model
  ModelFormat model_format = ModelFormat::AUTOREC;
};

/*! @brief Runtime object used to inference the loaded model on different devices
 */
struct FASTDEPLOY_DECL Runtime {
 public:
  /// Intialize a Runtime object with RuntimeOption
  bool Init(const RuntimeOption& _option);

  /** \brief Inference the model by the input data, and write to the output
   *
   * \param[in] input_tensors Notice the FDTensor::name should keep same with the model's input
   * \param[in] output_tensors Inference results
   * \return true if the inference successed, otherwise false
   */
  bool Infer(std::vector<FDTensor>& input_tensors,
             std::vector<FDTensor>* output_tensors);

  /** \brief No params inference the model.
   *
   *  the input and output data need to pass through the BindInputTensor and GetOutputTensor interfaces.
   */
  bool Infer();

  /** \brief Compile TorchScript Module, only for Poros backend
   *
   * \param[in] prewarm_tensors Prewarm datas for compile
   * \param[in] _option Runtime option
   * \return true if compile successed, otherwise false
   */
  bool Compile(std::vector<std::vector<FDTensor>>& prewarm_tensors,
               const RuntimeOption& _option);

  /** \brief Get number of inputs
   */
  int NumInputs() { return backend_->NumInputs(); }
  /** \brief Get number of outputs
   */
  int NumOutputs() { return backend_->NumOutputs(); }
  /** \brief Get input information by index
   */
  TensorInfo GetInputInfo(int index);
  /** \brief Get output information by index
   */
  TensorInfo GetOutputInfo(int index);
  /** \brief Get all the input information
   */
  std::vector<TensorInfo> GetInputInfos();
  /** \brief Get all the output information
   */
  std::vector<TensorInfo> GetOutputInfos();
  /** \brief Bind FDTensor by name, no copy and share input memory
   */
  void BindInputTensor(const std::string& name, FDTensor& input);
  /** \brief Get output FDTensor by name, no copy and share backend output memory
   */
  FDTensor* GetOutputTensor(const std::string& name);

  /** \brief Clone new Runtime when multiple instances of the same model are created
   *
   * \param[in] stream CUDA Stream, defualt param is nullptr
   * \return new Runtime* by this clone
   */
  Runtime* Clone(void* stream = nullptr,
                 int device_id = -1);

  RuntimeOption option;

 private:
  void CreateOrtBackend();
  void CreatePaddleBackend();
  void CreateTrtBackend();
  void CreateOpenVINOBackend();
  void CreateLiteBackend();
  void CreateRKNPU2Backend();
  std::unique_ptr<BaseBackend> backend_;
  std::vector<FDTensor> input_tensors_;
  std::vector<FDTensor> output_tensors_;
};
}  // namespace fastdeploy
