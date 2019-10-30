/*
 * This file is a part of the TChecker project.
 *
 * See files AUTHORS and LICENSE for copyright details.
 *
 */

#ifndef TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_ALGORITHM_HH
#define TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_ALGORITHM_HH

#include <functional>
#include <iterator>
#include <tuple>
#include <vector>
#include <atomic>

#include "tchecker/basictypes.hh"
#include "tchecker/algorithms/covreach/builder.hh"

#include "tchecker_ext/algorithms/covreach_ext/graph.hh"
#include "tchecker_ext/algorithms/covreach_ext/stats.hh"
#include "tchecker_ext/utils/show_stats.hh"

#include <tchecker_ext/config.hh>

#include <chrono>
#include <thread>

/*!
 \file algorithm.hh
 \brief Reachability algorithm with covering
 */

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    namespace helper{
      // Checks if a given value and a pointer to a given value are "always" equivalent
      template <class T>
      void is_constant_check(T val, T* val_ptr, std::atomic_bool *is_terminated, size_t wait=0, std::string description=""){
        while (!(*is_terminated)) {
          if (*val_ptr != val) {
            std::cout << "Faulty addr " << val_ptr << std::endl;
            throw std::runtime_error("value pointed to changed! : " + description);
          }
          if (wait>0){
            std::this_thread::sleep_for(std::chrono::nanoseconds(wait));
          }
        }
      }
  
      template <class NODE_PTR>
      void start_surv(std::thread &surv_thread, std::atomic_bool *is_terminated, const NODE_PTR &node, std::string description=""){
        
        std::cout << description << " : " << node.ptr() << " : " << (reinterpret_cast<unsigned int *>(node.ptr()) - 1) << std::endl;
        
        *is_terminated = false;
        surv_thread = std::thread(tchecker_ext::covreach_ext::helper::is_constant_check<unsigned int>,
            node->refcount(), (reinterpret_cast<unsigned int *>(node.ptr()) - 1), is_terminated, 0, description);
      }
  
      template <class NODE_PTR>
      size_t start_surv(std::vector<std::thread> &surv_thread_vec, std::vector<std::atomic_bool*> &is_terminated_vec, const NODE_PTR &node, std::string description=""){
        
        std::cout << description << " : " << node.ptr() << " : " << (reinterpret_cast<unsigned int *>(node.ptr()) - 1) << std::endl;
        
        size_t num = surv_thread_vec.size();
        std::atomic_bool *this_bool = new std::atomic_bool(false);
        is_terminated_vec.push_back(this_bool);
        surv_thread_vec.emplace_back(tchecker_ext::covreach_ext::helper::is_constant_check<unsigned int>,
                                     node->refcount(), (reinterpret_cast<unsigned int *>(node.ptr()) - 1),
                                     is_terminated_vec.back(), 0, description);
        return num;
      }
      template<class A, class B>
      void stop_last_surv(A &surv_thread_vec, B &is_terminated_vec){
        *is_terminated_vec.back() = true;
        surv_thread_vec.back().join();
        delete is_terminated_vec.back();
        is_terminated_vec.pop_back();
        surv_thread_vec.pop_back();
      }
    }
    
    
    namespace threaded_working{
      // Forward decl.
      template <class BUILDER, class GRAPH, class STATS>
      void expand_node(const int, const typename GRAPH::node_ptr_t &node, BUILDER &builder, const GRAPH &graph, std::vector<typename GRAPH::node_ptr_t> & nodes, STATS & stats);
      
      template <class GRAPH, class BUILDER, class WAITING, class ACCEPTING, class STATS>
      void worker_fun(const int worker_num, GRAPH & graph, BUILDER & builder, WAITING & waiting, ACCEPTING & accepting, STATS & stats, std::atomic_bool & is_reached) {
        using node_ptr_t = typename GRAPH::node_ptr_t;
        
//        std::vector<std::thread> comp_thread_vec, comp_thread_vec2;
//        std::vector<std::atomic_bool*> is_terminated_vec, is_terminated_vec2;

#if (SCHLEPIL_DBG>=1)
        std::cout << "worker " << worker_num << " starting off with visited " << stats.visited_nodes() << std::endl;
#endif
        
        node_ptr_t current_node{nullptr};
        std::vector<node_ptr_t> next_nodes;
        
        // Stop if some other thread reached the label
        assert(current_node.ptr() == nullptr);
        next_nodes.clear();
        while (!is_reached && waiting.pop_and_increment(current_node)) { //This no longer interferes with reference counter
          stats.increment_visited_nodes();
#if (SCHLEPIL_DBG>=1)
          std::cout << "worker " << worker_num << " got new current node " << current_node.ptr() << std::endl;
#endif
//          tchecker_ext::covreach_ext::helper::start_surv(comp_thread_vec, is_terminated_vec, current_node, "current node of " + std::to_string(worker_num));
          
          // Check if done
          if (accepting(current_node)) {
            // No successors of final state
            assert(next_nodes.empty());
            waiting.insert_and_decrement(next_nodes);
            // set the done "flag"
            is_reached = true;
            // all work is done
            std::cout << "worker " << worker_num << " reached final state" << std::endl;
            return;
          }
          // Building is thread safe as the allocation is
          // The nodes in next_nodes can currently only be accessed by this thread
          
          // TODO Currently we will also build successors of inactive nodes!
          assert(next_nodes.empty());
          tchecker_ext::covreach_ext::threaded_working::expand_node(worker_num, current_node, builder, graph, next_nodes, stats); //This interferes with the reference counter of next_nodes (not current_node!), however all the nodes are local, so no other thread knows about them yet
#if (SCHLEPIL_DBG>=2)
          std::cout << "worker " << worker_num << " next_nodes size after " << next_nodes.size() << std::endl;
#endif
    
          // This part is critical; graph sections have to be protected with locks
          // Till here no next_nodes are inserted in graph or waiting
//          tchecker_ext::covreach_ext::helper::stop_last_surv(comp_thread_vec, is_terminated_vec);
#if (SCHLEPIL_DBG>=1)
          std::cout << "worker " << worker_num << " inserting to graph " << current_node.ptr() << std::endl;
#endif
          graph.check_and_insert(current_node, next_nodes, stats);
//          for (const auto &v: next_nodes){
//            if (v.ptr() != nullptr){
//              std::cout << "child of " << worker_num << " : " << v.ptr() << std::endl;
//              tchecker_ext::covreach_ext::helper::start_surv(comp_thread_vec, is_terminated_vec, v, "child nodes of " + std::to_string(worker_num));
//            }
//          }
  
          assert(current_node.ptr() == nullptr); // Check and insert has to safely delete the reference to the parent
          // Those that are still active were added to the graph
          // It is no longer safe to simply clear the vector
    
          // next_nodes still holds all nodes, however is_active is correctly set for each element
          // active nodes will be automatically filtered
#if (SCHLEPIL_DBG>=1)
          std::cout << "worker " << worker_num << " inserting to waiting" << std::endl;
#endif
          waiting.insert_and_decrement(next_nodes, true, worker_num);
          // Done
          
          assert(next_nodes.empty());
//          while (!comp_thread_vec.empty()){
//            tchecker_ext::covreach_ext::helper::stop_last_surv(comp_thread_vec, is_terminated_vec);
//          }
        }
        std::cout << "worker " << worker_num << " terminates due to empty queue" << std::endl;
        return;
      }
  
      template <class BUILDER, class GRAPH, class STATS>
      void expand_node(const int worker_num, const typename GRAPH::node_ptr_t & node, BUILDER & builder, const GRAPH & graph, std::vector<typename GRAPH::node_ptr_t> & nodes, STATS & stats) {
        using node_ptr_t = typename GRAPH::node_ptr_t;
        using transition_ptr_t = typename GRAPH::ts_allocator_t::transition_ptr_t;

#if (SCHLEPIL_DBG>=1)
        std::cout << "worker " << worker_num << " expanding " << node.ptr() << std::endl;
#endif
  
  
  
//        std::atomic_bool is_terminated;
//        std::thread thread_compare;
//        tchecker_ext::covreach_ext::helper::start_surv(thread_compare, &is_terminated, node, "expanding node of " + std::to_string(worker_num));
  
        size_t nbr_children = 0;
        node_ptr_t next_node{nullptr};
        transition_ptr_t transition{nullptr};
        
        // Helper
        std::vector<std::pair<node_ptr_t, transition_ptr_t>> outgoing_range_alt;
        
        assert(nodes.empty());
    
        auto outgoing_range = builder.outgoing(node);
        outgoing_range_alt.clear();
//        builder.outgoing_alternative(node, outgoing_range_alt);
        for (auto it = outgoing_range.begin(); !it.at_end(); ++it) {
          nbr_children++; //todo beurk
        }
        nodes.reserve(nbr_children); // constructed with null_ptr
        for (auto it = outgoing_range.begin(); !it.at_end(); ++it) {
          std::tie(next_node, transition) = *it;
          assert(next_node.ptr() != nullptr);
//          nodes.push_back(next_node);
          // Construct a ptr that points to null; then it will be swapped with next_node
          nodes.emplace_back(nullptr);
          next_node.swap(nodes.back());
        }
        // Check for each node if covered by some node
        for (auto &node_a : nodes) {
          for (auto &node_b : nodes) {
            if (node_a == node_b) {
              // Do not compare to self
              continue;
            }
            if ( node_b->is_active() && (graph.is_le(node_a, node_b))) {
              node_a->make_inactive();
              // Once it is inactive -> continue
              stats.increment_directly_covered_leaf_nodes();
              break;
            }
          }
        }
#if (SCHLEPIL_DBG>=1)
        size_t active_node_dbg = 0;
        for (auto &node_a : nodes) {
          if(node_a->is_active()){
            active_node_dbg++;
          }
        }
        std::cout << "worker " << worker_num << " next_nodes size in fun " << nodes.size() << " with " << active_node_dbg << " active nodes" << std::endl;
#endif
//        is_terminated = true;
//        thread_compare.join();
        return;
      } //expand_node
      
    } // threaded_working
    
    /*!
     \class algorithm_t
     \brief Reachability algorithm with node covering
     \tparam TS : type of transition system, should derive from tchecker::ts::ts_t
     \tparam GRAPH : type of graph, should derive from tchecker::covreach::graph_t
     \tparam WAITING : type of waiting container, should derive from tchecker::covreach::active_waiting_t
     */
    template <class TS, class BUILD_ALLOC, class GRAPH, template <class NPTR> class WAITING>
    class algorithm_t {
      using ts_t = TS;
      using builder_alloc_t = BUILD_ALLOC;
      using transition_t = typename GRAPH::ts_allocator_t::transition_t;
      using transition_ptr_t = typename GRAPH::ts_allocator_t::transition_ptr_t;
      using graph_t = GRAPH;
      using ts_allocator_t = typename GRAPH::ts_allocator_t;
      using node_ptr_t = typename GRAPH::node_ptr_t;
      using edge_ptr_t = typename GRAPH::edge_ptr_t;
      //using builder_t = typename tchecker::covreach::builder_t<ts_t, ts_allocator_t>;
      using builder_t = typename tchecker::covreach::builder_t<ts_t, builder_alloc_t>;
    public:
      /*!
       \brief Reachability algorithm with node covering
       \param ts : a transition system
       \param graph : a graph
       \param accepting : an accepting function over nodes
       \pre accepting is monotonous w.r.t. the ordering over nodes in graph: if a node is accepting,
       then any bigger node is accepting as well (partially checked by assertion)
       \post this algorithm visits ts and builds graph. Graph stores the maximal nodes in ts and edges
       between them. There are two kind of edges: actual edges which correspond to a transition in ts,
       and abstract edges. There is an abstract edge n1->n2 when the actual successor of n1 in ts is
       smaller than n2 (for some n2 in the graph). The order in which the states of ts are visited
       depends on the policy implemented by WAITING.
       The algorithms stops when an accepting node has been found, or when the graph has been entirely
       visited.
       \return ACCEPTING if TS has an accepting run, NON_ACCEPTING otherwise
       \note this algorithm may not terminate if graph is not finite
       */
      std::tuple<enum tchecker::covreach::outcome_t, tchecker_ext::covreach_ext::stats_t>
      run(std::deque<TS> & ts_vec, std::deque<BUILD_ALLOC> & build_alloc_vec, GRAPH & graph, tchecker::covreach::accepting_labels_t<node_ptr_t> & accepting, const int num_threads)
      {
        //using accepting_t = tchecker::covreach::accepting_condition_t<node_ptr_t>;
        using accepting_t = tchecker::covreach::accepting_labels_t<node_ptr_t>;
        //using worker_t = tchecker_ext::covreach_ext::threaded_working::worker_t<ts_t, graph_t, WAITING>;
        
        //builder_t builder(ts, graph.ts_allocator());
        std::deque<builder_t> builder_vec;
        std::deque<tchecker::covreach::accepting_labels_t<node_ptr_t>> accepting_vec; //Avoids conversion to std::function
        
        WAITING<node_ptr_t> waiting;
        std::vector<node_ptr_t> nodes;
        std::deque<tchecker_ext::covreach_ext::stats_t> stats_vec;
        std::deque<std::thread> thread_vec;
        
        std::atomic_bool is_reached=false;
        std::atomic_bool do_show=true;
        
        tchecker::spinlock_t initial_lock;
        initial_lock.lock(); //Release before threads are launched
        
        
        // Create all builders based on helper allocators
        for (int i=0; i<num_threads; i++){
          //builder_vec.emplace_back(ts_vec[i], graph.ts_allocator());
          builder_vec.emplace_back(ts_vec[i], build_alloc_vec[i]);
          std::cout << "Builder address " << i << " : " << &builder_vec.back() << std::endl;
          stats_vec.emplace_back();
          accepting_vec.push_back(tchecker::covreach::accepting_labels_t<node_ptr_t>(accepting)); //Make sure they are copied
        }
        
        //Launch the show_stats
        //std::thread show_stats_thread(tchecker_ext::show_stats<std::deque<tchecker_ext::covreach_ext::stats_t>>, std::ref(stats_vec), std::ref(do_show), 1);
        
        // initial nodes
        // Set them before the threads are started
        nodes.clear();
        expand_initial_nodes(builder_vec.back(), graph, nodes);
        waiting.insert_and_decrement(nodes, false);
        
        // Make sure all nodes are properly set
        initial_lock.unlock(); // initial_lock is no longer necessary
        // Now the actual work can start
        
        // Get one stat for each thread
        // template <class GRAPH, class BUILDER, class WAITING, class ACCEPTING, class STATS>
        //void worker_fun(GRAPH & graph, BUILDER & builder, WAITING & waiting, ACCEPTING & accepting, STATS & stats, std::atomic_bool & is_reached)
        // Each thread gets a copy of builder. The allocation is thread safe, however (at least) the vm is not
        for (int i=0; i<num_threads-1; ++i){
          std::cout << "Thread " << i << " uses ts " << &ts_vec[i] << " and builder " << &builder_vec[i] << std::endl;
          thread_vec.emplace_back( tchecker_ext::covreach_ext::threaded_working::worker_fun<graph_t,
                                     builder_t, WAITING<node_ptr_t>, accepting_t, tchecker_ext::covreach_ext::stats_t>,
                                     i, std::ref(graph), std::ref(builder_vec[i]), std::ref(waiting), std::ref(accepting_vec[i]),
                                     std::ref(stats_vec[i]), std::ref(is_reached) );
        }
        
        // The last "thread" runs in the main thread
        // As this is blocking, we know when we are done
        std::cout << "Thread base uses ts " << &ts_vec.back() << " and builder " << &builder_vec.back() << std::endl;
        tchecker_ext::covreach_ext::threaded_working::worker_fun<graph_t, builder_t, WAITING<node_ptr_t>,
            accepting_t, tchecker_ext::covreach_ext::stats_t>(num_threads-1, graph, builder_vec.back(), waiting, accepting_vec.back(), stats_vec.back(), is_reached);
        
        // Wait till all are joined
        for (auto && it : thread_vec){
          it.join();
        }
        
        do_show=false;
        //show_stats_thread.join();
  
        tchecker_ext::covreach_ext::stats_t stat_tot(stats_vec);
        
        for (int i=0; i<num_threads; ++i){
          std::cout << "Stats for worker " << i << std::endl << stats_vec[i] << std::endl;
        }
        
        std::cout << "Total stats are " << std::endl << stat_tot << std::endl;
        
        return std::make_tuple(is_reached ? tchecker::covreach::REACHABLE : tchecker::covreach::UNREACHABLE, stat_tot);
      }
      
      /*!
       \brief Expand initial nodes
       \param builder : a transition system builder
       \param graph : a graph
       \param nodes : a vector of nodes
       \post the initial nodes provided by builder have been added to graph and to nodes
       */
      void expand_initial_nodes(tchecker::covreach::builder_t<TS, builder_alloc_t > & builder, GRAPH & graph,
                                std::vector<node_ptr_t> & nodes)
      {
        node_ptr_t node{nullptr};
        transition_ptr_t transition{nullptr};
        
        auto initial_range = builder.initial();
        for (auto it = initial_range.begin(); ! it.at_end(); ++it) {
          std::tie(node, transition) = *it;
          assert(node != node_ptr_t{nullptr});
          
          assert(node->is_active());//todo
          graph.add_node(node, GRAPH::ROOT_NODE);
          
          nodes.push_back(node);
        }
      }
    };
    
  } // end of namespace covreach
  
} // end of namespace tchecker

#endif // TCHECKER_EXT_ALGORITHMS_COVREACH_EXT_ALGORITHM_HH