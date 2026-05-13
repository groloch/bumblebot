# ifndef NN_EVALUATOR_H
# define NN_EVALUATOR_H

# include <array>
# include <cstdint>

# include <onnxruntime_cxx_api.h>

# include "nn/nn_utils.h"


namespace nn {

struct ModelOutput {
    std::array<float, 4288> policy;
    // Win probability in [0, 1] from player to move's perspective (sigmoid applied
    // model-side). Caller flips to side-to-move POV.
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
