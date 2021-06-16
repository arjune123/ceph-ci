// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:nil -*-
// vim: ts=8 sw=2 smarttab

#include "fatal_signal.h"

#include <csignal>
#include <iostream>
#include <string_view>

#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>
#include <seastar/core/reactor.hh>

#include "common/Thread.h"

FatalSignal::FatalSignal()
{
  install_oneshot_signals_handler<SIGSEGV,
                                  SIGABRT,
                                  SIGBUS,
                                  SIGILL,
                                  SIGFPE,
                                  SIGXCPU,
                                  SIGXFSZ,
                                  SIGSYS>();
}

static void reraise_fatal(const int signum)
{
  // Use default handler to dump core
  ::signal(signum, SIG_DFL);
  int ret = ::raise(signum);

  // Normally, we won't get here. If we do, something is very weird.
  char buf[1024];
  if (ret) {
    snprintf(buf, sizeof(buf), "reraise_fatal: failed to re-raise "
	    "signal %d\n", signum);
  } else {
    snprintf(buf, sizeof(buf), "reraise_fatal: default handler for "
	    "signal %d didn't terminate the process?\n", signum);
  }
  std::cerr << buf << std::flush;
  //::_exit(1);
}

template <int... SigNums>
void FatalSignal::install_oneshot_signals_handler()
{
  (install_oneshot_signal_handler<SigNums>() , ...);
}

template <int SigNum>
void FatalSignal::install_oneshot_signal_handler()
{
  struct sigaction sa;
  sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
    constexpr static pid_t NULL_TID{0};
    static std::atomic<pid_t> handler_tid{NULL_TID};
    if (auto expected{NULL_TID};
        !handler_tid.compare_exchange_strong(expected, ceph_gettid())) {
      if (expected == ceph_gettid()) {
        // The handler code may itself trigger a SIGSEGV if the heap is corrupt.
        // In that case, SIG_DFL followed by return specifies that the default
        // signal handler -- presumably dump core -- will handle it.
        ::signal(SigNum, SIG_DFL);
      } else {
        // Huh, another thread got into troubles while we are handling the fault.
        // If this is i.e. SIGSEGV handler, returning means retrying the faulty
        // instruction one more time, and thus all those extra threads will run
        // into a busy-wait basically.
      }
      return;
    }
    FatalSignal::signaled(sig, info);
    reraise_fatal(sig);
  };
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO | SA_NODEFER;
  if constexpr (SigNum == SIGSEGV) {
    sa.sa_flags |= SA_ONSTACK;
  }
  [[maybe_unused]] auto r = ::sigaction(SigNum, &sa, nullptr);
  assert(r == 0);
}


static void print_backtrace(std::string_view cause) {
  std::cerr << cause;
  if (seastar::engine_is_ready()) {
    std::cerr << " on shard " << seastar::this_shard_id();
  }
  std::cerr << ".\nBacktrace:\n";
  std::cerr << boost::stacktrace::stacktrace();
  std::cerr << std::flush;
  // TODO: dump crash related meta data to $crash_dir
  //       see handle_fatal_signal()
}

static void print_segv_info(const siginfo_t* siginfo)
{
  std::cerr << "Fault at location: " << siginfo->si_addr << std::endl;
  std::cerr << std::flush;
}

void FatalSignal::signaled(const int signum, const siginfo_t* siginfo)
{
  switch (signum) {
  case SIGSEGV:
    print_backtrace("Segmentation fault");
    print_segv_info(siginfo);
    break;
  case SIGABRT:
    print_backtrace("Aborting");
    break;
  default:
    print_backtrace(fmt::format("Signal {}", signum));
    break;
  }
}
