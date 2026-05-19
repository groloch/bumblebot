# ifndef NN_EVALUATOR_H
# define NN_EVALUATOR_H

# include <array>
# include <atomic>
# include <condition_variable>
# include <cstddef>
# include <cstdint>
# include <future>
# include <memory>
# include <mutex>
# include <thread>
# include <unordered_map>
# include <vector>

# include <onnxruntime_cxx_api.h>

# include "types.h"
# include "nn/nn_utils.h"


namespace nn {

struct ModelOutput {
    std::array<float, 4288> policy;
    float value;
};


class Evaluator {
public:
    static Evaluator& instance();

    std::shared_future<ModelOutput> submit(Hash key, ModelInput input);

    virtual std::vector<ModelOutput> evaluate_batch(ModelInput const* ins,
                                                    std::size_t count) = 0;

    void setBatchSize(unsigned n);
    void setMaxWaitUs(unsigned us);

    unsigned batchSize() const { return batchSize_.load(std::memory_order_relaxed); }
    unsigned maxWaitUs() const { return maxWaitUs_.load(std::memory_order_relaxed); }

    Evaluator(Evaluator const&) = delete;
    Evaluator& operator=(Evaluator const&) = delete;

protected:
    Evaluator();
    virtual ~Evaluator();

    void start_worker();
    void stop_worker();

private:
    void run();

    struct Request {
        Hash key;
        ModelInput input;
        std::shared_ptr<std::promise<ModelOutput>> promise;
    };

    std::mutex mu_;
    std::condition_variable cv_;
    std::vector<Request> queue_;
    std::unordered_map<Hash, std::shared_future<ModelOutput>> inflight_;

    std::atomic<unsigned> batchSize_{32};
    std::atomic<unsigned> maxWaitUs_{200};
    std::atomic<bool> stop_{false};

    std::thread worker_;
};


class OnnxEvaluator : public Evaluator {
public:
    OnnxEvaluator();
    ~OnnxEvaluator() override;

    std::vector<ModelOutput> evaluate_batch(ModelInput const* ins,
                                            std::size_t count) override;

private:
    Ort::Env env_;
    Ort::Session session_;
    Ort::MemoryInfo memInfo_;
};

}


# endif
