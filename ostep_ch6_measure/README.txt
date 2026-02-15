OSTEP Chapter 6 (Limited Direct Execution) — Measurement Homework Helpers
=======================================================================

This folder contains small C programs to help you measure:

  1) Timer precision / resolution
  2) Null-ish system-call cost
  3) Context-switch cost (pipe ping-pong), plus an optional baseline subtraction

They are written to be friendly to Ubuntu under WSL 2.

Build:
  make

Run examples:
  ./timer_precision
  ./syscall_cost --iters 20000000
  ./ctxswitch_cost --iters 2000000

Notes:
- Absolute numbers under WSL 2 can be noisier than bare metal because you're running
  inside a VM-like environment.
- For the context-switch test, the program pins both processes to CPU 0 using
  sched_setaffinity(); if that fails, it will print a warning and continue.

