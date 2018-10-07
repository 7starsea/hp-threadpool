high-performance-thread-pool(hp-threadpool)
=================
[![Build Status](https://travis-ci.org/7starsea/hp-threadpool.svg?branch=master)](https://travis-ci.org/7starsea/hp-threadpool)
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)

We have specific application (high-frequency trading system) in mind for designing the HPThreadPool. 
Common Features:

 * It is highly scalable and fast.
 * It is header only and lock-free.
 * No external dependencies, only standard library needed (C++11).

# How to use
You only need to include file **hp-threadpool.hpp** in your project. Please take a look at **example.cpp** to see how to use the APIs of **hp-threadpool.hpp**.

# Extra Contribution
We also provide a cross-platform **high_resolution_timer.h** for benchmark testing.

