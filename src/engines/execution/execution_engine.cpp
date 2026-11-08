#include "seventh/engines/execution/execution_engine.hpp"
#include <utility>
namespace seventh::execution {
void ExecutionEngine::register_executor(std::shared_ptr<OperationExecutor> executor) { dispatcher_.register_executor(std::move(executor)); }
ExecutionResult ExecutionEngine::execute(const ExecutionRequest& request) {
 ExecutionResult result;
 if(request.task_id.empty()||request.operation.empty()){result.error="Execution request is missing a task identifier or operation.";return result;}
 ExecutionRecord record; record.record_id=request.task_id; record.task_id=request.task_id; record.cir_id=request.cir_id; record.plan_id=request.plan_id;
 ExecutionStepRecord step; step.step_id=request.plan_step_id.empty()?request.task_id:request.plan_step_id; step.operation=request.operation; step.input_data=request.input_data;
 if(request.requires_authorization&&!request.authorization_granted){result.status=ExecutionStatus::NeedsPermission;result.error="This operation requires explicit authorization.";step.outcome=StepOutcome::Blocked;step.error=result.error;record.status=result.status;record.steps.push_back(std::move(step));records_[request.task_id]=std::move(record);return result;}
 if(!tracker_.begin(request.task_id)){result.error="This task is already running or has reached a terminal execution state.";step.outcome=StepOutcome::Blocked;step.error=result.error;record.status=result.status;record.steps.push_back(std::move(step));records_[request.task_id]=std::move(record);return result;}
 result=dispatcher_.dispatch(request);
 switch(result.status){
 case ExecutionStatus::Success: if(!tracker_.complete(request.task_id)){result.status=ExecutionStatus::Failure;result.error="Execution succeeded, but its tracker state could not be completed.";} break;
 case ExecutionStatus::NeedsPermission: if(!tracker_.block(request.task_id)){result.status=ExecutionStatus::Failure;result.error="Permission is required, but the tracker could not record the blocked state.";} break;
 case ExecutionStatus::Unsupported: case ExecutionStatus::Failure: if(!tracker_.fail(request.task_id))result.error+=" The tracker could not record the failed state."; break;
 }
 step.outcome=result.status==ExecutionStatus::Success?StepOutcome::Succeeded:(result.status==ExecutionStatus::NeedsPermission?StepOutcome::Blocked:StepOutcome::Failed);
 step.output_data=result.output_data; step.error=result.error; record.status=result.status; record.steps.push_back(std::move(step)); records_[request.task_id]=std::move(record); return result;
}
ExecutionState ExecutionEngine::state_of(const std::string& task_id) const{return tracker_.state_of(task_id);}
std::optional<ExecutionRecord> ExecutionEngine::record_of(const std::string& task_id) const{auto f=records_.find(task_id);if(f==records_.end())return std::nullopt;return f->second;}
}