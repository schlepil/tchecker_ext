//
// Created by philipp on 02.10.19.
//

#include <cstring>
#include <getopt.h>
#include <string>
#include <unordered_map>

#include "tchecker/utils/waiting.hh"
#include "tchecker_ext/algorithms/explore_ext/options.hh"
#include "tchecker_ext/algorithms/explore_ext/run.hh"
#include "tchecker/parsing/parsing.hh"
#include "tchecker/utils/log.hh"

#include "tchecker_ext/post_proc/post_proc_explore.hh"

using command_line_options_map_t = std::unordered_map<std::string, std::string>;
/*!
 \brief Parse command line options
 \param argc : number of command-line options
 \param argv : command-line options
 \param options : string of command-line options
 \param long_options : array of long names for command-line options
 \param log : logging facility
 \return A tuple <map, index> where map have entries (option, value) for every option with
 corresponding value from argv, and parsed according to options and long_options.
 value is the empty string if option has no associated value.
 index is the position of the first non-flag option in argv
 \pre options and long_options should follow getopt_long() requirements.
 Moreover, entries in long_options should either have format {long_name, arg, 0, short_name}
 or {long_name, arg, 0, 0} (no corresponding short name). Every short option (in options) must
 have a corresponding long option name in long_options
 argv[] is assumed to start with flags (of the form -s or --long), and then all non-flag
 options (input file names, etc)
 \post all errors and warnings have been reported to log
 */
std::tuple<command_line_options_map_t, int>
parse_options(int argc, char * argv[], char const * options, struct option const * long_options, tchecker::log_t & log)
{
  command_line_options_map_t map;
  int option = 0, option_index = 0;
  
  while (1) {
    option = getopt_long(argc, argv, options, long_options, &option_index);
    
    if (option == -1)
      break;        // All flags have been parsed
    
    if (option == ':')
      log.warning("Missing option parameter");
    else if (option == '?')
      log.warning("Unknown option");
    else if (option != 0) {
      char opt[] = {static_cast<char>(option), 0};
      map[opt] = (optarg == nullptr ? "" : optarg);
    }
    else
      map[long_options[option_index].name] = (optarg == nullptr ? "" : optarg);
  }
  
  return std::make_tuple(map, optind);
}

using zone_semantics_t = tchecker::zg::ta::elapsed_extraLUplus_local_t;
using explored_model_t = tchecker::explore::details::zg::ta::explored_model_t<zone_semantics_t>;

using model_t = tchecker::zg::ta::model_t;
using ts_t = explored_model_t::ts_t;
using node_t = explored_model_t::node_t;
using edge_t = explored_model_t::edge_t;
using hash_t = tchecker::intrusive_shared_ptr_delegate_hash_t;
using equal_to_t = tchecker::intrusive_shared_ptr_delegate_equal_to_t;
using graph_allocator_t = explored_model_t::graph_allocator_t;
using node_outputter_t = explored_model_t::node_outputter_t;
using edge_outputter_t = explored_model_t::edge_outputter_t;
using graph_outputter_t = tchecker::graph::dot_outputter_t<node_t, edge_t, node_outputter_t, edge_outputter_t>;
using graph_t = tchecker::explore::graph_t<graph_allocator_t, hash_t, equal_to_t, graph_outputter_t>;

template<class T>
using waiting_container_t= tchecker::fifo_waiting_t<T>;

using node_ptr_t = typename graph_t::node_ptr_t;

using zone_bare_t = typename node_t::zone_t;
using vloc_bare_t = typename node_t::state_t::vloc_t::object_t;
using intvar_bare_t = typename node_t::state_t::intvars_valuation_t::object_t;

//using node_bare_t = tchecker::zg::details::state_t<vloc_bare_t, intvar_bare_t, zone_bare_t,
using sol_graph_t = tchecker_ext::post_proc::solution_graph_t<graph_allocator_t>;


int main(int argc, char * argv[]){
  
  
  if (argc < 1) {
    return 1;
  }
  // parse options
  command_line_options_map_t map;
  int index;
  std::string filename("");
  tchecker::log_t log(&std::cerr);
  
  std::tie(map, index) = parse_options(argc, argv, tchecker_ext::explore_ext::options_t::getopt_long_options,
                                       tchecker_ext::explore_ext::options_t::getopt_long_options_long, log);
  
  
  if (index < argc) {
    if (index != argc - 1) {
      log.error("more than 1 input file provided");
      return -1;
    }
    
    filename = argv[index];
  }
  
  tchecker::parsing::system_declaration_t const * sysdecl = nullptr;
  
  try {
    sysdecl = tchecker::parsing::parse_system_declaration(filename, log);
    if (sysdecl == nullptr){
      throw std::runtime_error("nullptr system declaration");
    }
    tchecker_ext::explore_ext::options_t options(tchecker::make_range(map.begin(), map.end()), log);
  
    model_t model(*sysdecl, log);
    ts_t ts(model);
  
    tchecker::gc_t gc;
  
    graph_t graph(model.system().name(),
                  std::tuple<tchecker::gc_t &, std::tuple<model_t &, std::size_t>, std::tuple<>>
                      (gc, std::tuple<model_t &, std::size_t>(model, options.block_size()), std::make_tuple()),
                  options.output_stream(),
                  explored_model_t::node_outputter_args(model),
                  explored_model_t::edge_outputter_args(model));
  
    // Construct the accepting label
    tchecker::label_index_t label_index(model.system().labels());
    for (std::string const & label : options.accepting_labels()) {
      if (label_index.find_value(label) == label_index.end_value_map())
        label_index.add(label);
    }
  
    tchecker::covreach::accepting_labels_t<node_ptr_t> accepting_labels(label_index, options.accepting_labels());
  
    //sol_graph_t sol_graph; //For post processing
    std::vector<node_ptr_t> initial_nodes_vec; // Avoid having to reconstruct the initial nodes
    std::vector<node_ptr_t> accepting_nodes_vec; // Avoid having to reconstruct the initial nodes
  
    gc.start();
  
    tchecker_ext::explore_ext::algorithm_t<ts_t, graph_t, waiting_container_t> algorithm;
  
    try {
      algorithm.run(ts, graph, initial_nodes_vec, accepting_nodes_vec, accepting_labels);
    }
    catch (...) {
      gc.stop();
      graph.clear();
      graph.free_all();
      throw;
    }
    gc.stop();
    //Do the post-processing
    sol_graph_t sol_graph(32767, std::tuple<tchecker::gc_t &, std::tuple<model_t &, std::size_t>, std::tuple<>>
                                 (gc, std::tuple<model_t &, std::size_t>(model, options.block_size()), std::make_tuple()) );
    
    gc.start();
    for (const node_ptr_t & node_ptr : initial_nodes_vec){
      sol_graph.add_node(node_ptr, tchecker_ext::post_proc::node_types_t::INIT);
    }
    // TODO
//      for (auto && it : init_nodes_ptr){
//        std::cout <<  it.refcount() << std::endl;
//        sol_graph.add_node(it, tchecker::post_proc::INIT);
//        std::cout <<  it.refcount() << std::endl;
//      }
  
  
    gc.stop();
    initial_nodes_vec.clear();
    accepting_nodes_vec.clear();
    graph.free_all();
    sol_graph.free_all();
  }
  catch (std::exception const & e) {
    log.error(e.what());
  }
}