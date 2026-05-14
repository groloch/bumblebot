# ifndef NN_EVALUATOR_H
# define NN_EVALUATOR_H

# include <array>
# include <cstdint>

# include <onnxruntime_cxx_api.h>

# include "nn/nn_utils.h"


namespace nn {

struct ModelOutput {
    std::array<float, 4288> policy;
    float                   value;
};

class Evaluator {
public:
    static Evaluator& instance();

    ModelOutput evaluate(ModelInput const& in);

    Evaluator(Evaluator const&)            = delete;
    Evaluator& operator=(Evaluator const&) = delete;

private:
    Evaluator();

    Ort::Env        env_;
    Ort::Session    session_;
    Ort::MemoryInfo memInfo_;
};

}


# endif
