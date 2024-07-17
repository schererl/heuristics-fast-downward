from copy import deepcopy
import numpy as np
import pandas as pd
import csv


for ex in ['b']:
    infile = "results/{}.out".format(ex)
    print(infile)
    databases = []
    with open(infile, "r") as file:
        counter = 0
        data = {}
        read_instance = False
        read_implementation = False
        database = []
        code = None
        for line in file.readlines():
            
            # read a file name
            if read_implementation:
                data["implementation"] = line.replace("\n", "")
                read_implementation = False
            elif read_instance:
                data["instance"] = line.replace("\n", "").replace("castle/", "").replace("-cards.pddl", "")
                read_instance = False
                read_implementation = True
                 
            # reset variables to read new execution
            elif '@' in line and not counter == 0:
                if not code == 0:
                    data["h_init"] = -1
                    data["plan_cost"] = -1
                database.append(deepcopy(data))
                counter = 0
                data.clear()
                data["h_init"] = -1
                data["plan_cost"] = -1
                code = None
                read_instance = False
                
            # start reading an execution
            elif '@' in line and counter == 0:
                counter = 1
                read_instance = True
            elif "search exit code:" in line:
                code = int(line.replace(" ", "").split(":")[1])
            elif "Initial heuristic value for " in line:
                try:
                    data['h_init'] = int(line.replace("Initial heuristic value for ", "").replace('\n', '').split(' ')[-1])
                except:
                    data['h_init'] = np.inf
            elif "Search time" in line:
                data['search_time'] = float(line.replace("Search time: ", "").replace('s','').replace('\n', '').split(' ')[-1])
            elif "Expanded" in line:
                data['expanded_states'] = int(line.replace("Expanded ", "").replace(" state(s).", "").replace('\n', '').split(' ')[-1])
            elif "Plan cost: " in line:
                data['plan_cost'] = int(line.replace("Plan cost: ", "").replace('\n', ''))

    databases += database

    
    data_relaxed_opt = []
    data_opt_add = []
    data_opt_ff = []
    data_ff = []
    data_add = []
    data_opt_rtg = []
    data_blind = []
    for dict in databases:
        if dict['implementation'] == 'optimal':
            data_relaxed_opt.append(dict)
        elif dict['implementation'] == 'planopt_add':
            data_opt_add.append(dict)
        elif dict['implementation'] == 'planopt_ff':
            data_opt_ff.append(dict)
        elif dict['implementation'] == 'ff':
            data_ff.append(dict)
        elif dict['implementation'] == 'add':
            data_add.append(dict)
        elif dict['implementation'] == 'planopt_rtg':
            data_opt_rtg.append(dict)
        elif dict['implementation'] == 'blind':
            data_blind.append(dict)


    combined_data = {}
    for data_list in [data_relaxed_opt, data_opt_add, data_opt_ff, data_ff, data_add, data_opt_rtg, data_blind]:
        for entry in data_list:
            instance = entry['instance']
            if instance not in combined_data:
                combined_data[instance] = {}
            combined_data[instance][entry['implementation']] = entry


    output_data = []
    for instance, implementations in combined_data.items():
        for impl, data in implementations.items():
            output_data.append({
                'implementation': impl,
                'instance': data['instance'],
                'plan_cost': data['plan_cost'],
                'h_init': data['h_init'],
                'expanded_states': data.get('expanded_states', -1),  
                'search_time': data.get('search_time', -1)  
            })


    fields = ["implementation", "instance", "plan_cost", "h_init", "expanded_states", "search_time"]
    outfile = f"results/{ex}.csv"
    with open(outfile, "w") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fields)
        writer.writeheader()
        writer.writerows(output_data)

    # Load the CSV file into a DataFrame
    df = pd.read_csv(outfile)
    print(df)
