//
// Created by philipp on 24.09.19.
//

#ifndef TCHECKER_EXT_GRAPH_HH
#define TCHECKER_EXT_GRAPH_HH

#include <chrono>
#include <thread>

#include "tchecker/algorithms/covreach/graph.hh"

#include "tchecker_ext/algorithms/covreach_ext/waiting.hh"
#include "tchecker_ext/utils/spinlock.hh"

#include <tchecker_ext/config.hh>



namespace tchecker_ext{
  namespace covreach_ext{
    
    /*!
     \brief A partially thread safe version of tchecker::covreach::graph_t
     \note Attention this class is not inherently thread-safe; Only function where it is explicetly noted are thread-safe
     \tparam KEY
     \tparam TS
     \tparam TS_ALLOCATOR
     */
    template <class KEY, class TS, class TS_ALLOCATOR>
    class graph_t: public tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>{
    public:
  
      /*!
       \brief Type of pointers to node
       */
      using node_ptr_t = typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::node_ptr_t;
  
      template <class ... ARGS>
      graph_t(tchecker::gc_t & gc,
              std::tuple<ARGS...> && ts_alloc_args,
              std::size_t block_size,
              std::size_t table_size,
              typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::node_to_key_t node_to_key,
              typename tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::node_binary_predicate_t le_node)
              : tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>(gc, std::forward<std::tuple<ARGS...>>(ts_alloc_args), block_size, table_size, node_to_key, le_node)
          {
            _container_locks = std::vector<tchecker_ext::spinlock_t>(table_size);
          }
      
      /*!
       \brief Function that will decide whether a node is covered by the graph or is covering some in the graph
       \note The covered / covering nodes are all in the same container -> We can modify the edges of existing nodes
             The edges can be modified as they are newly created
       @tparam STATS
       @param next_nodes_vec
       @param stats
       */
      // TODO check if it wouldn't be more efficient to only keep incoming edges correct and add the outgoing edges afterwards
      template <class STATS>
      void check_and_insert(node_ptr_t &parent_node, std::vector<node_ptr_t> &next_nodes_vec, STATS &stats){
//        _glob_lock.lock();//testing
        // todo what happens if child covers parent?
        // We also need to lock the parent container
        
        // The idea is to loop over the different containers and take one that is currently free
        size_t no_access_counter=0;
        size_t no_access=0;
        size_t num_to_treat = 0;
        node_ptr_t covering_node{nullptr};
        node_ptr_t next_node{nullptr};
        std::vector<node_ptr_t> covered_nodes_vec;
        auto covered_nodes_vec_inserter = std::back_inserter(covered_nodes_vec);
        std::vector<tchecker::graph::cover::node_position_t> associated_container_num(next_nodes_vec.size());
        std::vector<bool> is_treated(next_nodes_vec.size(), true);
  
        tchecker::graph::cover::node_position_t parent_container_num =
            tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(parent_node);
        
        //Before starting check if the parent is still active
        _container_locks[parent_container_num].lock();
        if (!parent_node->is_active()){
          // Till here the next-nodes are only known to this thread
          // We can just delete the whole vector
          next_nodes_vec.clear();
          //Delete the reference to the parent
          parent_node = node_ptr_t{nullptr};
          // Unlock and return
          _container_locks[parent_container_num].unlock();
          _glob_lock.unlock();//testing
          return;
        }
        //Unlock if parent still active
        _container_locks[parent_container_num].unlock();
        
        // next_nodes are still thread local
        // Loop once to get all container id's and count how many are active
        for (size_t i=0; i < next_nodes_vec.size(); ++i){
          assert(next_nodes_vec[i]->refcount()==1);
          if (next_nodes_vec[i]->is_active()){
            num_to_treat++;
            is_treated[i] = false; // This node has to be handled
            associated_container_num[i] = tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(next_nodes_vec[i]); // This corresponds to the container id
          }else{
            // If they are inactive delete the reference
            next_nodes_vec[i] = node_ptr_t{nullptr};
          }
        }
  
        while (num_to_treat>0){ // Until all next_nodes_vec are treated Other loop to avoid dead-locks
          // Lock the parent
          // This can produce a deadlock
          // If all children and parents have the same states across all threads
          _container_locks[parent_container_num].lock();
          
          assert(covered_nodes_vec.empty());
          no_access_counter = 0;
          while (num_to_treat>0){ // Until all next_nodes_vec are treated
            no_access = 1;
            for (size_t i=0; i < next_nodes_vec.size(); ++i) { // Loop over next_nodes_vec to check which one we can treat next
              if (is_treated[i]){
                continue; //Already done
              }
              assert(next_nodes_vec[i].ptr()!=nullptr);
              // Check if the associated container is free AND acquire it
              // Except if it is the same container as parent, then ok...
              if (_container_locks[associated_container_num[i]].lock_once()
                  || (associated_container_num[i]==parent_container_num) ){
                // Check up
#if (SCHLEPIL_DBG >= 2)
                tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::check_container(associated_container_num[i]);
#endif
                
                no_access = 0; // Got access to at least one
//                next_node = next_nodes_vec[i];//
                //Swap them so that next_nodes_vec[i] becomes nullptr
                next_node.swap(next_nodes_vec[i]);

                // Mark as treated
                --num_to_treat;
                is_treated[i] = true;
                
                // Now we can treat the next_node as all the nodes that we have to compare it to are stored in this (now locked) container
                if (tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::is_covered_external(next_node, covering_node)){ //covered?
                  assert( (tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(next_node) ==
                              tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(covering_node)) );
                  //next_node->make_inactive();
                  //next_node is not yet stored in graph -> remove it
                  next_node = node_ptr_t{nullptr};
                  covering_node = node_ptr_t{nullptr};
                  
                  // TODO
                  //tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_edge(parent_node, covering_node, tchecker::covreach::ABSTRACT_EDGE);
                  stats.increment_covered_leaf_nodes();
                }else{ // covering?
                  // Now we are sure that the node is not included in some other node
                  // and we will add it to the graph along with the edge
                  assert(next_node->is_active()); //Todo
#if (SCHLEPIL_DBG>0)
                  assert( (_container_locks[tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::get_node_position(next_node)].lock_once() == false) );
#endif
                  tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_node(next_node); //From now on others threads can possible see it if the corresponding container is unlocked
                  assert(next_node->is_active()); //Todo
                  // #TODO
                  //tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::add_edge(parent_node, next_node, tchecker::covreach::ACTUAL_EDGE);
                  
                  // Check if this new node covers others
                  assert(covered_nodes_vec.empty());
                  tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::covered_nodes(next_node, covered_nodes_vec_inserter); //next_node and covered nodes are in the same container so we can change the reference counter
                  //for (covered_node : covered_nodes_vec) {
                  for (size_t j=0; j<covered_nodes_vec.size(); ++j ){
#if (SCHLEPIL_DBG>0)
                    tchecker::graph::cover::node_position_t  this_pos =
                      tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::position_in_table(covered_nodes_vec[j]);
                    assert( this_pos == associated_container_num[i]);
                    assert( _container_locks[this_pos].lock_once() == false );
#endif
                    covered_nodes_vec[j]->make_inactive(); // This ensure that it is not enlisted in waiting afterwards
                    cover_node(covered_nodes_vec[j], next_node);
                    stats.increment_covered_nonleaf_nodes();
                  }// covered
                  covered_nodes_vec.clear(); //Clear before releasing the container
                  // Swap it back into the vector as this node remains active
                  next_node.swap(next_nodes_vec[i]);
                }//covering
                
                //Make sure all ptr as reset before releasing the container
                assert(covered_nodes_vec.empty());
                assert(next_node.ptr() == nullptr);
                assert(covering_node.ptr() == nullptr);
                // Check up
#if (SCHLEPIL_DBG>=2)
                tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::check_container(associated_container_num[i]);
#endif
                
                // Finished treating this node
                // Unlock, except if parent
                if(associated_container_num[i]!=parent_container_num){
                  _container_locks[associated_container_num[i]].unlock(); // Release container
                }
              } // if locked
            } // for next_node : next_nodes_vec
            no_access_counter += no_access;
            
            if (no_access_counter>100){//todo
#if (SCHLEPIL_DBG>=2)
              std::cout << "breaking inner loop - probably a dead-lock" << std::endl;
#endif
              _container_locks[parent_container_num].unlock();//Release container
              // Give other threads a chance
              std::this_thread::sleep_for(std::chrono::microseconds(5));
              break;
            }
          } // while untreated
        } // while untreated deadlocks
  
        // Also safely release the reference to the parent
        parent_node = node_ptr_t{nullptr};
        //Release parent container
        _container_locks[parent_container_num].unlock();

#if (SCHLEPIL_DBG>=1)
        for (size_t i=0; i < next_nodes_vec.size(); ++i) {
          assert((next_nodes_vec[i].ptr() == nullptr) || (next_nodes_vec[i]->refcount() == 2));
        }
#endif
//        _glob_lock.unlock();//testing
        return;
      }//check_and_insert
      
      /*!
       \brief Cover a node
       \param covered_node : covered node
       \param covering_node : covering node
       \post graph has been updated to let covering_node replace covered_node
       */
      void cover_node(node_ptr_t & covered_node, node_ptr_t & covering_node)
      {
#if (SCHLEPIL_DBG>=2)
        tchecker::graph::cover::node_position_t  this_pos = tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::position_in_table(covered_node);
        assert(!_container_locks[this_pos].lock_once()); //Check that locked
#endif
        // #todo
        //tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::move_incoming_edges(covered_node, covering_node, tchecker::covreach::ABSTRACT_EDGE);
        //tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::remove_edges(covered_node);
        tchecker::covreach::graph_t<KEY, TS, TS_ALLOCATOR>::remove_node(covered_node);
        
        return;
      }

    private:
      // TODO the locks should probably go to cover/graph for more coherence
      std::vector<tchecker_ext::spinlock_t> _container_locks; /*! One lock for each node_ptr_t container */
      tchecker_ext::spinlock_t _glob_lock; //Testing
      
    };
    
  }//covreach_ext
}//tchecker_ext

#endif //TCHECKER_EXT_GRAPH_HH