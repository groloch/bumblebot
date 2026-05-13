# include "nn/evaluator.h"

# include <cstring>


namespace nn {

namespace {

constexpr char MODEL_PATH[] = "model.onnx";

constexpr int64_t TOKENS_LEN = 70;
constexpr int64_t POLICY_LEN = 4288;

// Must match the names emitted by torch.onnx.export on the trainer side.
constexpr const char* INPUT_NAMES[]  = {"x", "hm", "epsq"};
constexpr const char* OUTPUT_NAMES[] = {"policy", "value"};

Ort::SessionOptions makeSessionOptions(){
    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(1);
    opts.AppendExecutionProvider("MIGraphX", {{"migraphx_fp16_enable", "1"}});
    return opts;
}

}


Evaluator& Evaluator::instance(){
    static Evaluator inst{};
    return inst;
}

Evaluator::Evaluator()
    : env_{ORT_LOGGING_LEVEL_WARNING, "bumblebot"},
      session_{env_, MODEL_PATH, makeSessionOptions()},
      memInfo_{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)}
{}

ModelOutput Evaluator::evaluate(ModelInput const& in){
    const int64_t tokensShape[] = {1, TOKENS_LEN};
    const int64_t scalarShape[] = {1};

    Ort::Value inputs[] = {
        Ort::Value::CreateTensor<int64_t>(memInfo_,
            const_cast<int64_t*>(in.tokens.data()), TOKENS_LEN,
            tokensShape, 2),
        Ort::Value::CreateTensor<float>(memInfo_,
            const_cast<float*>(&in.hm), 1,
            scalarShape, 1),
        Ort::Value::CreateTensor<int64_t>(memInfo_,
            const_cast<int64_t*>(&in.epsq), 1,
            scalarShape, 1),
    };

    auto outs = session_.Run(Ort::RunOptions{nullptr},
                             INPUT_NAMES, inputs, 3,
                             OUTPUT_NAMES, 2);

    ModelOutput out;
    std::memcpy(out.policy.data(), outs[0].GetTensorData<float>(),
                POLICY_LEN * sizeof(float));
    out.value = *outs[1].GetTensorData<float>();
    return out;
}

}
