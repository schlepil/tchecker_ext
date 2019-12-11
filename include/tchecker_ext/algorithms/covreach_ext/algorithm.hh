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

#include <tchecker_ext/config.hh>

#include <chrono>
#include <thread>

/*!
 \file algorithm.hh
 \brief Reachability algorithm with covering
 */

namespace tchecker_ext {
  
  namespace covreach_ext {
    
    namespace threaded_working{
      // Forward decl.
      template <class BUILDER, class GRAPH, class STATS>
      void expand_node(const int, const typename GRAPH::node_ptr_t &node, BUILDER &builder, const GRAPH &graph, std::vector<typename GRAPH::node_ptr_t> & nodes, STATS & stats);
      
      /*!
       * struct working_elements
       * \brief Helper structure that contains all vectors
       * that are needed within the exploration of the transition system
       * and the construction of the graph
       * \tparam NODE_PTR
       * \note This avoids reallocation of memory for the vectors
       * as they are reused
       */
      template <class NODE_PTR>
      struct working_elements{
        std::vector<NODE_PTR> next_nodes_vec, covered_nodes_vec;
        std::vector<tchecker::graph::cover::node_position_t> associated_container_num;
        std::vector<bool> is_treated;
      };
      
      
      /*!
       * \brief This is the main function executed by each thread to explore the zone graph
       * @tparam GRAPH
       * @tparam BUILDER
       * @tparam WAITING
       * @tparam ACCEPTING
       * @tparam STATS
       * @param worker_num the identifier of this thread
       * @param graph the graph to be constructed. Needs to be thread safe, tchecker_ext::covreach_ext::graph_t
       * nodes sharing a vector each have a lock
       * @param builder The builder of the ts. Needs to be thread safe -> Each builder needs its own VM and a
       *                specialization of the allocator to avoid singleton allocation problem
       * @param waiting A thread-safe waiting structure: pushing and popping needs to be locked; Due to
       * reference counter cannot use active waiting list
       * @param accepting A callable object or function that takes a node and determines whether it is accepting
       * @param stats Use a vector of stats, one for each threads
       * @param is_reached An atomic flag to signal termination among threads
       * \note thread-safe here means is more "strict" then traditional thread-safe, as the reference counter of each
       *       object is not thread-safe. Therefore the reference counter may only change when the corresponding object
       *       is locked
       */
      template <class GRAPH, class BUILDER, class WAITING, class ACCEPTING, class STATS>
      void worker_fun(const int worker_num, GRAPH & graph, BUILDER & builder, WAITING & waiting, ACCEPTING & accepting,
          STATS & stats, std::atomic_bool & is_reached) {
        using node_ptr_t = typename GRAPH::node_ptr_t;
        
        working_elements<node_ptr_t> this_work_elems;
        node_ptr_t current_node{nullptr};
        std::vector<node_ptr_t> &next_nodes_vec = this_work_elems.next_nodes_vec;
        
        // Create a builder function
        // Building as such is thread safe, but it is better to pass
        // the builder function to the graph so that the activeness
        // of the parent_node can be verified
        std::function<void(node_ptr_t const &)> build_exp_node =
            [&] (node_ptr_t const & node) {
                return expand_node(worker_num, node,
                    builder, graph, next_nodes_vec, stats);
        };
        
        // Stop if some other thread reached the label
        next_nodes_vec.clear();
        while (!is_reached && waiting.pop_and_increment(current_node)) {

          // Check if done
          if (accepting(current_node)) {
            stats.increment_visited_nodes();
            // No successors of final state
            assert(next_nodes_vec.empty());
            waiting.insert_and_decrement(next_nodes_vec);
            // set the done "flag"
            is_reached = true;
            // all work is done
            std::cout << "worker " << worker_num << " reached final state" << std::endl;
            return;
          }
          
          // This part is critical
          //graph.check_and_insert(current_node, next_nodes, stats);
          assert(next_nodes_vec.empty());
          graph.build_and_insert(current_node, build_exp_node, this_work_elems, stats);
  
          assert(current_node.ptr() == nullptr); // Check and insert has to safely delete the reference to the parent
          // Those that are still active were added to the graph
          // It is no longer safe to simply clear the vector ->
          // swap them into the waiting list as this does not impact the reference counter
    
          waiting.insert_and_decrement(next_nodes_vec, true);
          // Done
          assert(next_nodes_vec.empty());
        }
        if(is_reached){
          std::cout << "worker " << worker_num << " terminates because another thread reached the goal" << std::endl;
        }else{
          std::cout << "worker " << worker_num << " terminates due to empty queue" << std::endl;
        }
        return;
      }
  
      
      /*!
       * \brief Computes the successors of a given node; In order to be thread safe, the graph is not allowed
       * to be modified (only used to compare the nodes),
       * The reference counter of the parent node is not allowed to change
       * @tparam BUILDER builder with threaded allocation for the transitions (singleton_pool_allocator ...)
       * @tparam GRAPH
       * @tparam STATS
       * @param worker_num
       * @param node
       * @param builder
       * @param graph
       * @param nodes vector of nodes to store the successors; Has to be empty on call
       * @param stats
       */
      template <class BUILDER, class GRAPH, class STATS>
      void expand_node(const int worker_num, const typename GRAPH::node_ptr_t & node,
           BUILDER & builder, const GRAPH & graph, std::vector<typename GRAPH::node_ptr_t> & nodes,
           STATS & stats) {
        using node_ptr_t = typename GRAPH::node_ptr_t;
        using transition_ptr_t = typename GRAPH::ts_allocator_t::transition_ptr_t;
        
        assert(nodes.empty());
        
        node_ptr_t next_node{nullptr};
        transition_ptr_t transition{nullptr};
        
        auto outgoing_range = builder.outgoing(node);
  
        // todo Do something to determine the size of a range
        // this way copying the vector when expanding it could be avoided
        // New elements are constructed with constructed with null_ptr
        for (auto it = outgoing_range.begin(); !it.at_end(); ++it) {
          std::tie(next_node, transition) = *it;
          assert(next_node.ptr() != nullptr);
          // Swaps a null_reference with the next_node
          nodes.emplace_back(nullptr);
          next_node.swap(nodes.back());
        }
        // Check for each node if covered by some other node
        // in nodes, called "direct_covering"
        for (auto &node_a : nodes) {
          for (const auto &node_b : nodes) {
            if (node_a == node_b) {
              // Do not compare to self
              continue;
            }
            if ( node_b->is_active() && (graph.is_le(node_a, node_b))) {
              node_a->make_inactive();
              // Once it is inactive -> continue
              stats.increment_directly_covered_leaf_nodes();
              break;
            } // if
          } // for node_b
        } // for node_a
      } //expand_node
      
    } // threaded_working
    
    /*!
     \class algorithm_t
     \brief Reachability algorithm with node covering and possible multi-threading
     \tparam TS : type of transition system, should derive from tchecker::ts::ts_t
     \tparam GRAPH : type of graph, should derive from tchecker_ext::covreach_ext::graph_t
     \tparam WAITING : type of waiting container, should derive from tchecker_ext::covreach_ext::threaded_waiting_t
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
      using builder_t = typename tchecker::covreach::builder_t<ts_t, builder_alloc_t>;
    public:
      /*!
       \brief Multithreaded reachability algorithm with node covering
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
       visited.
       \return ACCEPTING if TS has an accepting run, NON_ACCEPTING otherwise
       \note this algorithm may not terminate if graph is not finite
       */
      std::tuple<enum tchecker::covreach::outcome_t, tchecker_ext::covreach_ext::stats_t>
      run(std::deque<TS> & ts_vec, std::deque<BUILD_ALLOC> & build_alloc_vec, GRAPH & graph,
          tchecker::covreach::accepting_labels_t<node_ptr_t> & accepting,
          const unsigned int num_threads, const unsigned int n_notify)
      {
        using accepting_t = tchecker::covreach::accepting_labels_t<node_ptr_t>;
        
        std::deque<builder_t> builder_vec;
        // Todo change this such that all threads can share one accepting object
        // this facilitates the implementation of fastest trace etc
        std::deque<tchecker::covreach::accepting_labels_t<node_ptr_t>> accepting_vec; //Avoids conversion to std::function
        
        WAITING<node_ptr_t> waiting;
        std::vector<node_ptr_t> nodes;
        std::deque<tchecker_ext::covreach_ext::stats_t> stats_vec; // One stat per thread
        std::deque<std::thread> thread_vec;

        // "Flag" to signal whether some thread found an accepting node
        std::atomic_bool is_reached=false;
        
        tchecker::spinlock_t initial_lock;
        // Release before threads are launched
        // This is necessary as if one compiles with optimizations
        // the threads can be launched before all initial nodes are inserted
        initial_lock.lock();
        
        
        // Create all builders based on helper allocators
        for (unsigned int i=0; i<num_threads; i++){
          builder_vec.emplace_back(ts_vec[i], build_alloc_vec[i]);
          std::cout << "Builder address " << i << " : " << &builder_vec.back() << std::endl;
          //todo change this to be contained in options
          stats_vec.emplace_back(n_notify, "Visited nodes by thread " + std::to_string(i) + " : ");
          accepting_vec.push_back(tchecker::covreach::accepting_labels_t<node_ptr_t>(accepting)); //Make sure they are copied
        }
        
        // initial nodes
        // Set them before the threads are started
        nodes.clear();
        expand_initial_nodes(builder_vec.back(), graph, nodes);
        waiting.insert_and_decrement(nodes, false);
        
        // Make sure all nodes are properly set
        initial_lock.unlock(); // initial_lock is no longer necessary
        // Now the actual work can start
        
        // Each thread gets his own builder that share the allocator provided by the graph.
        // The allocation is thread safe, care has to be taken that there are never two or more threads that work
        // can work (modify the reference counter of) the same node.
        // The builder vm is not thread safe.
        for (unsigned int i=0; i<num_threads-1; ++i){
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
        for (auto & it : thread_vec){
          it.join();
        }
        
        tchecker_ext::covreach_ext::stats_t stat_tot(stats_vec);
        
        return std::make_tuple(is_reached ? tchecker::covreach::REACHABLE : tchecker::covreach::UNREACHABLE, stat_tot);
      }
      
      /*!f
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