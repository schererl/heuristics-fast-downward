#ifndef PLANOPT_HEURISTICS_RELAXED_TASK_GRAPH_H
#define PLANOPT_HEURISTICS_RELAXED_TASK_GRAPH_H

#include "and_or_graph.h"
#include "relaxed_task.h"

namespace planopt_heuristics {

class RelaxedTaskGraph {
    RelaxedTask relaxed_task;
    std::vector<NodeID> variable_node_ids;
    AndOrGraph graph;
    NodeID initial_node_id;
    NodeID goal_node_id;

    utils::HashMap<NodeID, std::string> map_id_name; //added for debugging

    std::vector<int> visited;
    // incremental visited, serve as a control for visited nodes into ff: 
    //   avoid reseting every time visited also avoid use set, from time to time reset it
    int v_incr; 
public:
    RelaxedTaskGraph(const TaskProxy &task_proxy);

    void change_initial_state(const GlobalState &global_state);

    bool is_goal_relaxed_reachable();
    
    void check_valuation_result();
    
    int additive_cost_of_goal();
    int ff_cost_of_goal();
};

}

#endif
