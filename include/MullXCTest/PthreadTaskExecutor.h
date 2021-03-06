#ifndef MULL_XCTEST_PTHREAD_TASK_EXECUTOR
#define MULL_XCTEST_PTHREAD_TASK_EXECUTOR

#include <llvm/ADT/FunctionExtras.h>
#include <llvm/ADT/ScopeExit.h>
#include <llvm/Support/Errno.h>
#include <llvm/Support/Error.h>
#include <mull/Diagnostics/Diagnostics.h>
#include <mull/Metrics/MetricsMeasure.h>
#include <mull/Parallelization/Progress.h>
#include <pthread.h>
#include <thread>

namespace mull {
std::vector<int> taskBatches(size_t itemsCount, size_t tasks);
void printTimeSummary(Diagnostics &diagnostics, MetricsMeasure measure);
} // namespace mull

namespace mull_xctest {

using namespace mull;

static inline bool MakeErrMsg(std::string *ErrMsg, const std::string &prefix,
                              int errnum = -1) {
  if (!ErrMsg)
    return true;
  if (errnum == -1)
    errnum = errno;
  *ErrMsg = prefix + ": " + llvm::sys::StrError(errnum);
  return true;
}

static inline void ReportErrnumFatal(const char *Msg, int errnum) {
  std::string ErrMsg;
  MakeErrMsg(&ErrMsg, Msg, errnum);
  llvm::report_fatal_error(ErrMsg);
}

using AsyncThreadInfo = llvm::unique_function<void()>;

static inline void *threadFuncAsync(void *Arg) {
  std::unique_ptr<AsyncThreadInfo> task(static_cast<AsyncThreadInfo *>(Arg));
  (*task)();
  return nullptr;
}

template <typename Task> class PthreadTaskExecutor {
public:
  using In = typename Task::In;
  using Out = typename Task::Out;
  PthreadTaskExecutor(Diagnostics &diagnostics, std::string name,
                      llvm::Optional<unsigned> StackSizeInBytes, In &in,
                      Out &out, std::vector<Task> tasks)
      : diagnostics(diagnostics), in(in), out(out), tasks(std::move(tasks)),
        name(std::move(name)), StackSizeInBytes(StackSizeInBytes) {}

  void execute() {
    if (tasks.empty() || in.empty()) {
      return;
    }

    measure.start();
    if (tasks.size() == 1 || in.size() == 1) {
      executeSequentially();
    } else {
      executeInParallel();
    }
    measure.finish();
    printTimeSummary(diagnostics, measure);
  }

private:
  void executeInParallel() {
    assert(tasks.size() != 1);
    assert(in.size() != 1);
    auto workers = std::min(in.size(), tasks.size());

    auto batches = taskBatches(in.size(), workers);
    std::vector<pthread_t> threads;
    std::vector<Out> storages;

    storages.reserve(workers);
    counters.reserve(workers);
    int errnum;

    auto end = in.begin();
    for (unsigned i = 0; i < workers; i++) {
      int group = batches[i];
      storages.push_back(Out());
      counters.push_back(progress_counter());

      auto begin = end;
      std::advance(end, group);

      pthread_attr_t attr;
      if ((errnum = ::pthread_attr_init(&attr)) != 0) {
        ReportErrnumFatal("pthread_attr_init failed", errnum);
      }
      auto attrGuard = llvm::make_scope_exit([&] {
        if ((errnum = ::pthread_attr_destroy(&attr)) != 0) {
          ReportErrnumFatal("pthread_attr_destroy failed", errnum);
        }
      });

      if (StackSizeInBytes) {
        if ((errnum = ::pthread_attr_setstacksize(&attr, *StackSizeInBytes)) !=
            0) {
          ReportErrnumFatal("pthread_attr_setstacksize failed", errnum);
        }
      }

      auto taskFn =
          new AsyncThreadInfo([task = std::move(tasks[i]), begin, end,
                               storageRef = std::ref(storages.back()),
                               counter = std::ref(counters.back())]() mutable {
            task(begin, end, storageRef, counter);
          });

      pthread_t thread;
      if ((errnum =
               ::pthread_create(&thread, &attr, threadFuncAsync, taskFn)) != 0)
        ReportErrnumFatal("pthread_create failed", errnum);

      threads.push_back(std::move(thread));
    }

    std::thread reporter(
        progress_reporter{diagnostics, name, counters, in.size(), workers});

    for (auto &t : threads) {
      if ((errnum = ::pthread_join(t, nullptr)) != 0) {
        ReportErrnumFatal("pthread_join failed", errnum);
      }
    }
    reporter.join();

    for (auto &storage : storages) {
      for (auto &m : storage) {
        out.push_back(std::move(m));
      }
    }
  }

  void executeSequentially() {
    assert(tasks.size() == 1 || in.size() == 1);
    auto &task = tasks.front();

    counters.push_back(progress_counter());
    std::thread reporter(
        progress_reporter{diagnostics, name, counters, in.size(), 1});
    task(in.begin(), in.end(), out, std::ref(counters.back()));
    reporter.join();
  }

  Diagnostics &diagnostics;
  In &in;
  Out &out;
  std::vector<Task> tasks;
  std::vector<progress_counter> counters{};
  MetricsMeasure measure;
  std::string name;
  llvm::Optional<unsigned> StackSizeInBytes;
};

} // namespace mull_xctest

#endif
