// ============================================================================
//  debug.h — Minimal, header-only debug logging utilities for the TIPS project
// ----------------------------------------------------------------------------
// MSU CSE 4714/6714 Capstone Project (Fall 2025)
// Author: Derek Willis
//
//  YOU SHOULD NOT NEED TO EDIT THIS FILE FOR ANY REASON
//
//  What this is:
//    A tiny, thread-safe switch and two helpers to print debug traces to
//    std::cerr. Because everything is inline in a header, you can include this
//    from any .cpp without needing a separate .o file.
//
//  Why this design:
//    • Uses a function-local static std::atomic<bool> to avoid static
//      initialization order issues across translation units.
//    • memory_order_relaxed is sufficient here because we only need a
//      best-effort “is debugging on?” flag (no inter-thread data handoff).
//    • Writes go to std::cerr so they don’t mix with normal program output.
//
//  Quick usage:
//      #include "debug.h"
//      dbg::set(true);               // turn debugging on
//      dbg::log("Parsing... ");      // prints without newline if enabled
//      dbg::line("done.");           // prints with newline if enabled
//
//  Notes for students:
//    • These calls become no-ops when debugging is off, so you can leave them in.
//    • Prefer dbg::line() for whole messages; use dbg::log() to build a line
//      across multiple calls.
//    • If you need compile-time removal of debug code, use macros and #ifdef;
//      this header is intentionally runtime-toggleable for teaching.
//
// ============================================================================

#pragma once
#include <iostream>
#include <atomic>

namespace dbg {

  /// Returns a reference to a process-wide atomic flag indicating whether
  /// debug logging is enabled. Implemented with a function-local static to
  /// guarantee initialization on first use (avoids static-order fiasco).
  inline std::atomic<bool>& enabled() {
    static std::atomic<bool> on{false};
    return on;
  }

  /// Toggle debug logging at runtime.
  /// Using memory_order_relaxed because we only read/write an independent flag.
  inline void set(bool on) {
    enabled().store(on, std::memory_order_relaxed);
  }

  /// Print a value to std::cerr without a trailing newline if debugging is on.
  /// Accepts any type that supports operator<<(std::ostream&, T).
  template<class T>
  inline void log(const T& x) {
    if (enabled().load(std::memory_order_relaxed)) std::cerr << x;
  }

  /// Print a value followed by '\n' to std::cerr if debugging is on.
  /// Handy for one-line status messages (e.g., “parsed stmt OK”).
  template<class T>
  inline void line(const T& x) {
    if (enabled().load(std::memory_order_relaxed)) std::cerr << x << '\n';
  }

} // namespace dbg
