#include "relaxed_task_graph.h"

#include <iostream>
#include <vector>
#include <deque>

using namespace std;

namespace planopt_heuristics {
RelaxedTaskGraph::RelaxedTaskGraph(const TaskProxy &task_proxy)
    : relaxed_task(task_proxy),
      variable_node_ids(relaxed_task.propositions.size()) {
    /*
      TODO: add your code for exercise 2 (b) here. Afterwards
        - variable_node_ids[i] should contain the node id of the variable node for variable i
        - initial_node_id should contain the node id of the initial node
        - goal_node_id should contain the node id of the goal node
        - the graph should contain precondition and effect nodes for all operators
        - the graph should contain all necessary edges.
    */
    
    // create variable nodes for each proposition
    for (size_t i = 0; i < relaxed_task.propositions.size(); ++i) {
        variable_node_ids[i] = graph.add_node(NodeType::OR, 0);
        map_id_name[variable_node_ids[i]] = relaxed_task.propositions[i].name;
    }
   
    // initial state
    initial_node_id = graph.add_node(NodeType::AND, 0);
    map_id_name[initial_node_id] = "INITIAL";
    
    // add edges from the initial node to the initial state variables
    for (PropositionID id : relaxed_task.initial_state) {
        graph.add_edge(variable_node_ids[id], initial_node_id);
    }

    // create the goal node
    goal_node_id = graph.add_node(NodeType::AND, 0);
    map_id_name[goal_node_id] = "GOAL";

    // add edges from the goal state variables to goal node
    for (PropositionID id : relaxed_task.goal) {
        graph.add_edge(goal_node_id, variable_node_ids[id]);
    }

    // add operators, precondition and effect nodes
    for (const RelaxedOperator &op : relaxed_task.operators) {
        // create a precondition  
        NodeID precond_node_id = graph.add_node(NodeType::AND, 0);
        map_id_name[precond_node_id] = "prec-"+op.name;
        // add edges from precondition to the variables
        for (PropositionID prop_id : op.preconditions) {
            graph.add_edge(precond_node_id, variable_node_ids[prop_id]);
        }

        // create an effect node with the operator's cost
        NodeID effect_node_id = graph.add_node(NodeType::AND, 0);
        map_id_name[effect_node_id] = "eff-"+op.name;
        // add edges from the effect node to variables
        for (PropositionID prop_id : op.effects) {
            graph.add_edge(variable_node_ids[prop_id], effect_node_id);
            
        }

        NodeID operator_node_id = graph.add_node(NodeType::AND, op.cost);
        map_id_name[operator_node_id]="O-"+op.name;
        //add edge connecting operator with preconditions
        graph.add_edge(operator_node_id, precond_node_id);
        //add edge connecting effect with the operator
        graph.add_edge(effect_node_id, operator_node_id);
        
    }

    //setup ff stuff
    visited.resize(graph.n_size(), -1);
    v_incr = 0;
}

void RelaxedTaskGraph::check_valuation_result() {
    // Display detailed info for each node type: Variable, Initial, Goal, Precondition, and Effect.
    std::cout << "Checking valuation results:\n" << std::endl;
    
    auto print_node_details = [&](const NodeID& node_id, const string& prefix = "") {
        const auto& node = graph.get_node(node_id);
        string node_status = node.forced_true ? "True" : "False";
        string node_type = (node.type == NodeType::AND) ? "AND" : "OR";
        string node_add_cost = std::to_string(node.additive_cost);
        std::cout << prefix << map_id_name.at(node_id) << " (ID: " << node_id << ", Type: " << node_type << ", Status: " << node_status << ", ADD Cost:" << node_add_cost << ")\n";
    };
    
    // Initial Node
    std::cout << "Initial State Node:\n";
    print_node_details(initial_node_id, "\t");
    std::cout << "\tSuccessors:\n";
    for (NodeID succ_id : graph.get_node(initial_node_id).successor_ids) {
        print_node_details(succ_id, "\t\t");
    }
    
    // Goal Node
    std::cout << "\nGoal State Node:\n";
    print_node_details(goal_node_id, "\t");
    std::cout << "\tPredecessors:\n";
    for (NodeID pred_id : graph.get_node(goal_node_id).predecessor_ids) {
        print_node_details(pred_id, "\t\t");
    }
    
    // Variable, Precondition, and Effect Nodes
    std::cout << "\nVariable, Precondition, and Effect Nodes:\n";
    for (const auto& entry : map_id_name) {
        // Skip initial and goal nodes to avoid repetition
        if (entry.first == initial_node_id || entry.first == goal_node_id || !graph.get_node(entry.first).forced_true) continue;
        
        print_node_details(entry.first, "\t");
        
        const auto& node = graph.get_node(entry.first);
        if (!node.predecessor_ids.empty()) {
            std::cout << "\t\tPredecessors:\n";
            for (NodeID pred_id : node.predecessor_ids) {
                print_node_details(pred_id, "\t\t\t");
            }
        }
        if (!node.successor_ids.empty()) {
            std::cout << "\t\tSuccessors:\n";
            for (NodeID succ_id : node.successor_ids) {
                print_node_details(succ_id, "\t\t\t");
            }
        }
    }
}

void RelaxedTaskGraph::change_initial_state(const GlobalState &global_state) {
    // Remove all initial edges that where introduced for relaxed_task.initial_state.
    for (PropositionID id : relaxed_task.initial_state) {
        graph.remove_edge(variable_node_ids[id], initial_node_id);
    }

    // Switch initial state of relaxed_task
    relaxed_task.initial_state = relaxed_task.translate_state(global_state);

    // Add all initial edges for relaxed_task.initial_state.
    for (PropositionID id : relaxed_task.initial_state) {
        graph.add_edge(variable_node_ids[id], initial_node_id);
    }
}

bool RelaxedTaskGraph::is_goal_relaxed_reachable() {
    // Compute the most conservative valuation of the graph and use it to
    // return true iff the goal is reachable in the relaxed task.
    graph.most_conservative_valuation();
    return graph.get_node(goal_node_id).forced_true;
}

int RelaxedTaskGraph::additive_cost_of_goal() {
    // Compute the weighted most conservative valuation of the graph and use it
    // to return the h^add value of the goal node.
    // TODO: add your code for exercise 2 (c) here.

    // Perform the weighted most conservative valuation first.
    graph.weighted_most_conservative_valuation();
    
    //std::cout << "\n\tNODE: " << graph.get_node(goal_node_id).additive_cost << std::endl;
    //exit(0);
    //check_valuation_result();
    // Return the additive cost of reaching the goal node.
    // This value was computed during the weighted most conservative valuation.
    return graph.get_node(goal_node_id).additive_cost;
}

// python3 fast-downward/build.py; python3 fast-downward/fast-downward.py castle/domain.pddl castle/castle-5-4-9-cards.pddl --heuristic "h=planopt_ff()" --search "eager_greedy([h])"  
int RelaxedTaskGraph::ff_cost_of_goal() {
    graph.weighted_most_conservative_valuation();
    int ff_value = 0;
    
    deque<NodeID> q({goal_node_id});
    
    while (!q.empty())
    {
        NodeID current_id = q.front();
        AndOrGraphNode current_node = graph.get_node(current_id);
        q.pop_front();
        // if visited is lower than v_incr, it wasn't visited in the current iteration
        if(visited[current_id] == v_incr){
            continue;
        }
        visited[current_id] = v_incr;
        ff_value += current_node.direct_cost;
        
        if (current_node.type == NodeType::OR)
            q.push_back(current_node.achiever);

        if (current_node.type == NodeType::AND)
            for (NodeID current_node_sucessor_id : current_node.successor_ids)
                q.push_back(current_node_sucessor_id);
    }
    
    v_incr+=1;
    // reset the incremental visited control
    // if(v_incr == 1000){
    //     v_incr = 0;
    //     std::fill(visited.begin(), visited.end(), -1);
    // }

    return ff_value;
}


}