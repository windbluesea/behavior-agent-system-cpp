#include "bas/inference/model_runtime.hpp"

namespace bas {

void ModelRuntime::Configure(const ModelConfig& config) {
  config_ = config;
}

ModelResponse ModelRuntime::RankAndExplain(const ModelRequest& request) const {
  ModelResponse response;
  if (request.candidate_summaries.empty()) {
    response.selected_index = 0;
    response.explanation = "no candidate decisions available";
    return response;
  }

  // Placeholder ranker: real integration should call local 1.5B runtime.
  response.selected_index = 0;
  response.explanation = "selected candidate 0 with deterministic baseline; model=" + config_.model_name +
                         (config_.use_int8 ? ", int8=true" : ", int8=false");
  return response;
}

}  // namespace bas
