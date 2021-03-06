#include "training/graph_group_singleton.h"
#include "tensors/tensor_operators.h"

namespace marian {

void SingletonGraph::setScheduler(Ptr<Scheduler> scheduler) {
  scheduler_ = scheduler;
  // optimizer has to be registered last to see changes of learning rate
  scheduler_->registerTrainingObserver(scheduler_);
  scheduler_->registerTrainingObserver(opt_);
}

void SingletonGraph::execute(Ptr<data::Batch> batch) {
  auto costNode = builder_->build(graph_, batch);

  graph_->forward();
  float cost = costNode->scalar();
  graph_->backward();

  // Get batch stats
  size_t batch_words = batch->wordsTrg();

  if(scaleLearningRate_) {
    opt_->update(graph_, batch_words / avgBatchWords_);
  } else {
    opt_->update(graph_);
  }

  if(mvAvg_) {
    ABORT_IF(!scheduler_, "Scheduler is required for exponential smoothing");

    if(!graphAvg_) {
      graphAvg_ = New<ExpressionGraph>();
      graphAvg_->setDevice(graph_->getDevice());
      graphAvg_->copyParams(graph_);
    } else {
      updateAvgParams(graphAvg_->params()->vals(),
                      graph_->params()->vals(),
                      scheduler_->numberOfBatches());
    }
  }

  if(scheduler_) {
    scheduler_->update(cost, batch);

    if(scheduler_->saving())
      this->save();

    if(scheduler_->validating()) {
      if(mvAvg_) {
        graphAvg_->reuseWorkspace(graph_);
        scheduler_->validate({graphAvg_});
      } else {
        scheduler_->validate({graph_});
      }
    }
  }
}
}  // namespace marian
