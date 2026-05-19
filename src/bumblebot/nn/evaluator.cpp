# include "nn/evaluator.h"

# include <algorithm>
# include <chrono>
# include <cstring>
# include <iterator>
# include <utility>

# include "position.h"
# include "debugging.h"


namespace nn {

namespace {

constexpr char MODEL_PATH[] = "model.onnx";

constexpr int64_t TOKENS_LEN = 70;
constexpr int64_t POLICY_LEN = 4288;

constexpr const char* INPUT_NAMES[] = {"x", "hm", "epsq"};
constexpr const char* OUTPUT_NAMES[] = {"policy", "value"};

Ort::SessionOptions makeSessionOptions(){
    Ort::SessionOptions opts;
    opts.SetIntraOpNumThreads(1);
    opts.AppendExecutionProvider("MIGraphX", {{"migraphx_fp16_enable", "1"}});
    return opts;
}

}


Evaluator& Evaluator::instance(){
    static OnnxEvaluator inst{};
    return inst;
}


Evaluator::Evaluator() = default;

Evaluator::~Evaluator(){
    stop_worker();
}


void Evaluator::setBatchSize(unsigned n){
    if(n == 0) n = 1;
    batchSize_.store(n, std::memory_order_relaxed);
    cv_.notify_all();
}


void Evaluator::setMaxWaitUs(unsigned us){
    maxWaitUs_.store(us, std::memory_order_relaxed);
    cv_.notify_all();
}


std::shared_future<ModelOutput> Evaluator::submit(Hash key, ModelInput input){
    std::unique_lock<std::mutex> lk{mu_};
    auto it = inflight_.find(key);
    if(it != inflight_.end()){
        return it->second;
    }
    auto promise = std::make_shared<std::promise<ModelOutput>>();
    std::shared_future<ModelOutput> future{promise->get_future().share()};
    inflight_.emplace(key, future);
    queue_.push_back(Request{key, std::move(input), promise});
    lk.unlock();
    cv_.notify_one();
    return future;
}


void Evaluator::start_worker(){
    worker_ = std::thread{&Evaluator::run, this};
}


void Evaluator::stop_worker(){
    {
        std::lock_guard<std::mutex> lk{mu_};
        stop_.store(true, std::memory_order_relaxed);
    }
    cv_.notify_all();
    if(worker_.joinable()){
        worker_.join();
    }
}


void Evaluator::run(){
    std::vector<Request> batch;
    std::vector<ModelInput> inputs;

    while(true){
        batch.clear();
        inputs.clear();

        unsigned bs{0};

        {
            std::unique_lock<std::mutex> lk{mu_};
            cv_.wait(lk, [&]{
                return stop_.load(std::memory_order_relaxed) || !queue_.empty();
            });
            if(stop_.load(std::memory_order_relaxed)) return;

            bs = batchSize_.load(std::memory_order_relaxed);
            const unsigned wait{maxWaitUs_.load(std::memory_order_relaxed)};

            if(queue_.size() < bs && wait > 0){
                cv_.wait_for(lk, std::chrono::microseconds{wait}, [&]{
                    return stop_.load(std::memory_order_relaxed)
                        || queue_.size() >= bs;
                });
                if(stop_.load(std::memory_order_relaxed)) return;
            }

            const std::size_t take{std::min<std::size_t>(queue_.size(), bs)};
            batch.reserve(take);
            std::move(queue_.begin(), queue_.begin() + take, std::back_inserter(batch));
            queue_.erase(queue_.begin(), queue_.begin() + take);

            for(Request const& r : batch){
                inflight_.erase(r.key);
            }
        }

        inputs.reserve(bs);
        for(Request const& r : batch){
            inputs.push_back(r.input);
        }
        while(inputs.size() < bs){
            inputs.push_back(batch[0].input);
        }

        std::vector<ModelOutput> outs{evaluate_batch(inputs.data(), inputs.size())};

        for(std::size_t i{0}; i < batch.size(); ++i){
            batch[i].promise->set_value(std::move(outs[i]));
        }
    }
}


OnnxEvaluator::OnnxEvaluator()
    : env_{ORT_LOGGING_LEVEL_WARNING, "bumblebot"},
      session_{env_, MODEL_PATH, makeSessionOptions()},
      memInfo_{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)}
{
    start_worker();
}


OnnxEvaluator::~OnnxEvaluator(){
    stop_worker();
}


std::vector<ModelOutput> OnnxEvaluator::evaluate_batch(ModelInput const* ins,
                                                       std::size_t count){
    assert(count > 0, "evaluate_batch: empty batch");

    const int64_t N{static_cast<int64_t>(count)};
    const int64_t tokensShape[]{N, TOKENS_LEN};
    const int64_t scalarShape[]{N};

    std::vector<int64_t> tokensBuf(static_cast<std::size_t>(N) * TOKENS_LEN);
    std::vector<Ort::Float16_t> hmBuf(count);
    std::vector<int64_t> epsqBuf(count);
    for(std::size_t i{0}; i < count; ++i){
        for(int64_t j{0}; j < TOKENS_LEN; ++j){
            tokensBuf[i * TOKENS_LEN + j] = ins[i].tokens[j];
        }
        hmBuf[i] = Ort::Float16_t{ins[i].hm};
        epsqBuf[i] = ins[i].epsq;
    }

    Ort::Value inputs[]{
        Ort::Value::CreateTensor<int64_t>(memInfo_,
            tokensBuf.data(), tokensBuf.size(),
            tokensShape, 2),
        Ort::Value::CreateTensor<Ort::Float16_t>(memInfo_,
            hmBuf.data(), hmBuf.size(),
            scalarShape, 1),
        Ort::Value::CreateTensor<int64_t>(memInfo_,
            epsqBuf.data(), epsqBuf.size(),
            scalarShape, 1),
    };

    auto outs = session_.Run(Ort::RunOptions{nullptr},
                             INPUT_NAMES, inputs, 3,
                             OUTPUT_NAMES, 2);

    std::vector<ModelOutput> results(count);
    const Ort::Float16_t* policyHalf{outs[0].GetTensorData<Ort::Float16_t>()};
    const Ort::Float16_t* valueHalf{outs[1].GetTensorData<Ort::Float16_t>()};
    for(std::size_t i{0}; i < count; ++i){
        for(int64_t j{0}; j < POLICY_LEN; ++j){
            results[i].policy[j] = static_cast<float>(policyHalf[i * POLICY_LEN + j]);
        }
        results[i].value = static_cast<float>(valueHalf[i]);
    }
    return results;
}

}
