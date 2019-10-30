//
// Created by philipp on 14.10.19.
//

#ifndef TCHECKER_EXT_SHOW_STATS_HH
#define TCHECKER_EXT_SHOW_STATS_HH

#include <chrono>
#include <thread>

namespace tchecker_ext{
  
  template<class VEC>
  void show_stats(const VEC & vec, std::atomic_bool & do_show, size_t interval=5){
    size_t counter;
    while (do_show){
      counter = 0;
      for (const auto & v: vec){
        std::cout << "Info for " << counter << std::endl;
        std::cout << v << std::endl;
      }
      std::this_thread::sleep_for(std::chrono::seconds(interval));
    }
    return;
  }
}

#endif //TCHECKER_EXT_SHOW_STATS_HH
