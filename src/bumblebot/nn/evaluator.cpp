# include "nn/evaluator.h"

# include <cstring>


namespace nn {

namespace {

constexpr char MODEL_PATH[] = "model.onnx";

constexpr int64_t TOKENS_LEN = 70;
constexpr int64_t POLICY_LEN = 4288;

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

    Ort::Float16_t hm16{in.hm};

    Ort::Value inputs[] = {
        Ort::Value::CreateTensor<int64_t>(memInfo_,
            const_cast<int64_t*>(in.tokens.data()), TOKENS_LEN,
            tokensShape, 2),
        Ort::Value::CreateTensor<Ort::Float16_t>(memInfo_,
            &hm16, 1,
            scalarShape, 1),
        Ort::Value::CreateTensor<int64_t>(memInfo_,
            const_cast<int64_t*>(&in.epsq), 1,
            scalarShape, 1),
    };

    auto outs = session_.Run(Ort::RunOptions{nullptr},
                             INPUT_NAMES, inputs, 3,
                             OUTPUT_NAMES, 2);

    ModelOutput out;
    const Ort::Float16_t* policyHalf{outs[0].GetTensorData<Ort::Float16_t>()};
    for(int64_t i{0}; i < POLICY_LEN; ++i){
        out.policy[i] = static_cast<float>(policyHalf[i]);
    }
    out.value = static_cast<float>(*outs[1].GetTensorData<Ort::Float16_t>());
    return out;
}

}
