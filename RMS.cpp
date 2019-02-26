#include <iostream>
#include <fstream> // file IO
#include <vector>  
#include <map>

using namespace std;

#define INT_MAX 2147483647

// General information of the cyclic process
struct process_info
{
	string pid;
	int processing_time;//burst time
	int period;//deadline
	int repeat;
	int total_waiting_time;
	int deadline_miss_count;
};

// Information in each cycle of a process
struct process
{
	string pid;
	int iteration;//cycle count
	int remaining_time;//remaining time left for the cycle to finish
	int system_joining_time;//The time at which the cycle joined the ready queue
};

class rm
{
	private :
	
	int timer;//time metric
	int time_max;//max possible time for simulation
	map<string,process_info> all_processes_hashmap; // maps process_id's to general process data
	map<string,process> ready_hashmap; // maps process id's to cyclic process data
	map<int,vector<process> > time_process_map; // maps time to a list of incoming processes
	ofstream log; //output file for logging
	ofstream stat;  //output file for stats

	// This functions checks if the process p's period predecessor has missed it's deadline and then proceeds to insert p in ready_processes 
	void insert_into_ready(process p);
	string highest_priority_from_ready(); // Get highest priority process from ready_processes
	void execute_window(string& running_process,int next_time); // Execute running_process in the given time window
	void get_stats(); // writes overall statistic metrics to stat file
	void display_time_map(); // function to display info in time_process_map 

	public:

	rm(vector<process_info> v)
	{
		// building all_process_hashmap
		for(int i=0;i<v.size();i++)
		{
			v[i].total_waiting_time = 0;
			v[i].deadline_miss_count = 0;
			all_processes_hashmap[v[i].pid]=v[i];
		}
		// building time_process_map and finding time_max
		time_max = 0;
		for(int i=0;i<v.size();i++)
		{
			// visit all k multiples of each process's period 
			// a dummy process is inserted at k+1th multiple to handle deadlines
			int time = 0;
			for(int count = 1; count <= v[i].repeat + 1 ; count++)
			{
				process p;
				p.pid = v[i].pid;
				p.iteration = count;
				p.remaining_time = v[i].processing_time;
				p.system_joining_time = time;
				time_process_map[time].push_back(p);
				time_max = (time_max > time)?time_max:time;
				time += v[i].period;
			}
		}
		timer = 0;
		//open output files
		log.open("RM-Log.txt");
		stat.open("RM-Stats.txt");
	}
	
	void start()
	{
		string running_process = "None";
		// move across time in discrete steps by iterating over all keys of time_process_map
		map<int,vector<process> > :: iterator it = time_process_map.begin();
		while(1)
		{
			timer = it->first;
			vector<process> entering_processes = it->second;
			//check for deadline missed and insert all the entering processes into the ready queue 
			for(int i=0;i<entering_processes.size();i++)
			{
				insert_into_ready(entering_processes[i]);		
			}
			//stop simuation at time_max
			if(timer == time_max)
				break;
			//Execute the running_process from now to next_time
			it++;
			int next_time = it->first;
			execute_window(running_process,next_time);
		}
		// Collect statistics once simulation is done
		get_stats();
		log.close();
		stat.close();
	}
};

void rm :: execute_window(string& running_process,int next_time)
{
	// Stop execution when next_time is reached or if there are no processes in ready_queue
	while(timer <= next_time && ready_hashmap.size()>0)
	{
		// Get the highest priority process
		string best_process = highest_priority_from_ready();
		if(running_process == "None")
		{
			if(ready_hashmap[best_process].remaining_time == all_processes_hashmap[best_process].processing_time)
				log<<"Process "<<best_process<<" starts execution at time "<<timer<<endl;
			else
				log<<"Process "<<best_process<<" resumes execution at time "<<timer<<endl;
		}
		else if (best_process != running_process)
		{
			// best_process preempts the running_process
			log<<"Process "<<running_process<<" is preempted by Process "<<best_process<<" at time "<<timer<<".";
			log<<"Remaining process time:"<<ready_hashmap[running_process].remaining_time<<endl;
			
			if(ready_hashmap[best_process].remaining_time == all_processes_hashmap[best_process].processing_time)
				log<<"Process "<<best_process<<" starts execution at time "<<timer<<endl;
			else
				log<<"Process "<<best_process<<" resumes execution at time "<<timer<<endl;
		}
		else{}
		// Set the current running_process to best_process and store it's remaining time and system_joining_time	
		running_process = best_process;
		int rem_time = ready_hashmap[running_process].remaining_time;
		int join_time = ready_hashmap[running_process].system_joining_time;
		
		if(timer + rem_time  >= next_time)
		{
			// This case implies the running_process shall completely run till next_time , decrease it's remaining time accordingly
			ready_hashmap[running_process].remaining_time -= (next_time-timer);
			break;
		}
		// This case implies the running_process shall finish before nex_time arrives , So remove the process from ready queue and update timer
		ready_hashmap.erase(running_process);
		log<<"Process "<<running_process<<" finished execution at time "<<timer +rem_time<<endl;
		all_processes_hashmap[running_process].total_waiting_time += timer+rem_time-join_time- all_processes_hashmap[running_process].processing_time;
		running_process = "None";
		timer = timer + rem_time ;
	}
	// If the loop breaks because there are no processes in ready queue , it means CPU shall be idle till next_time
	if(ready_hashmap.size() == 0)
	{
		log<<"CPU is idle till time "<<next_time-1<<endl;
	}
	return;
}

// This functions checks if the process p's period predecessor has missed it's deadline and then proceeds to insert p in ready_processes 
void rm :: insert_into_ready(process p)
{
	process_info p_info = all_processes_hashmap[p.pid];
	// Check if p.pid already exists in ready_process
	if( ready_hashmap.find(p.pid) != ready_hashmap.end() )
	{
		// This case implies p_prev missed it's deadline , replace it with p and update the deadline_miss_count for p.pid
		process p_prev = ready_hashmap[p.pid]; 
		log<<"Process "<<p.pid<<": remaining time="<<p_prev.remaining_time<<"; deadline:"<<p_info.period*(p_prev.iteration)<<" missed it's deadline and has been removed from the system at time "<<timer<<endl;
		all_processes_hashmap[p.pid].deadline_miss_count += 1;
		ready_hashmap[p.pid] = p;
		log<<"Process "<<p.pid<<": processing time="<<p_info.processing_time<<"; deadline:"<<p_info.period*(p.iteration)<<" joined the system at time "<<p.system_joining_time<<endl;	
		return;
	}
	// Stop the insertion of p, if it is a dummy process . The dummy process is the last iteration of it's kind
	if(p.iteration == p_info.repeat + 1)
	{
		return;
	}
	// insertion of p into ready
	ready_hashmap[p.pid] = p;
	log<<"Process "<<p.pid<<": processing time="<<p_info.processing_time<<"; deadline:"<<p_info.period*(p.iteration)<<" joined the system at time "<<p.system_joining_time<<endl;
	return;
}


// Get highest priority process from ready_processes
string rm :: highest_priority_from_ready()
{
	// In RMS , lower the period higher the priority
	string best_process = "None";
	int least_period = INT_MAX;
	// Iterate over all the ready processes and find the one with highest priotity
	for(map<string,process> :: iterator it = ready_hashmap.begin();it != ready_hashmap.end();it++)
	{
		string current_pid = it->first;
		int current_period = all_processes_hashmap[current_pid].period;
		if(current_period < least_period)
		{
			least_period = current_period;
			best_process = current_pid;
		}
	}
	return best_process;
}

void rm :: get_stats ()
{
	// compute and display all the stats
	int process_count = 0;
	int deadline_miss_count = 0;
	for(map<string,process_info> :: iterator it = all_processes_hashmap.begin();it != all_processes_hashmap.end();it++)
	{
		process_info p = it->second;
		process_count += p.repeat;
		deadline_miss_count += p.deadline_miss_count;
	}
	stat<<"Total process count : "<<process_count<<endl;
	stat<<"Total succesful process count : "<<process_count - deadline_miss_count<<endl;
	stat<<"Total deadline missed process count : "<<deadline_miss_count<<endl;
	for(map<string,process_info> :: iterator it = all_processes_hashmap.begin();it != all_processes_hashmap.end();it++)
	{
		process_info p = it->second;
		process_count += p.repeat;
		deadline_miss_count += p.deadline_miss_count;
		float avg_wt = p.total_waiting_time;
		// average waiting time is the total waiting time divided by successfully finsihed processes
		avg_wt = avg_wt/(p.repeat - p.deadline_miss_count);
		stat<<p.pid<<" average waiting time : "<<avg_wt<<endl;
	}
	return;
}

// A function to display contents of time_process_map , for debugging purposes 
void rm :: display_time_map()
{
	for(map<int,vector<process> > :: iterator it = time_process_map.begin();it!=time_process_map.end();it++)
	{
		
		for(int i = 0; i<it->second.size();i++)
		{
			log<<it->first<<" : ";
			log<<it->second[i].pid<<" "<<it->second[i].iteration<<" "<<it->second[i].remaining_time<<" "<<it->second[i].system_joining_time<<" ";
			process_info p_info = all_processes_hashmap[it->second[i].pid];
			log<<p_info.repeat<<endl;
		}
		log<<endl;
	}
}
int main()
{	
	ifstream input_file;
	input_file.open("inp-params.txt");
	
	// Read input and build a list of process_info
	vector<process_info> input_processes;
	int n;
	input_file>>n;
	while(n--)
	{
		process_info p;
		input_file>>p.pid>>p.processing_time>>p.period>>p.repeat;
		input_processes.push_back(p);
	}
	input_file.close();	

	//Invoke the rm constructor
	rm simulation(input_processes);
	//Run the simulation
	simulation.start();

	return 0;
}