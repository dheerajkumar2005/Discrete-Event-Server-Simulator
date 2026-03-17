#include <bits/stdc++.h>
using namespace std;

enum EventType {
    REQUEST_ARRIVAL = 1,
    REQUEST_DEPARTURE = 2,
    REQUEST_TIMEOUT = 3,
    CONTEXT_SWITCH = 4
};

struct Config {
    int num_users;
    double think_time_mean;
    double think_time_std;
    double timeout;
    int num_cores;
    int tot_threads;
    int queue_capacity;
    double service_time_mean;
    double quantum_time_slice;
    
    Config();
};

struct Metrics {
    double avg_response;
    double throughput;
    double goodput;
    double badput;
    double utilization;
    double drop_rate;
    double avg_num_system;
    double avg_queue_length;
    
    Metrics();
};

struct Request {
    int req_id;
    int user_id;
    bool timed_out;
    bool dropped;
    
    // timestamps
    double issue_time;
    double thread_assigned_time;
    double completion_time;
    
    // service parameters
    double total_service_time;
    double service_time_completed;
    double timeout_time;
    
    Request(int r_id, int u_id, double issue);
};

struct Event {
    double time;
    int event_type;
    int request_id;
    int core_id;
    int server_id;
    
    Event(double t, int type, int req = -1, int core = -1, int server_id = 0) ;
    bool operator<(const Event& other) const;
};

//global variables;
extern priority_queue<Event> event_heap;
extern vector<Request> requests;          // store all requests for further statistics calculation;
extern int global_request_counter;
extern int MAX_REQUESTS;
extern int completed_requests;
extern double current_time;
extern int WARMUP_REQUESTS;
extern int measured_completions;
extern int good_completions;
extern int bad_completions;
extern double warmup_end_time;
extern double total_core_busy_time;
extern int measured_arrivals;
extern int dropped_requests;
extern double last_event_time;
extern double area_num_system;
extern double area_queue_length;
extern default_random_engine generator;

//distribution functions;
double gaussian_sample(double mean, double stddev);
double exponential_sample(double mean);

class User {
    public :
    int user_id;
    double think_time_mean;
    double think_time_std;
    double timeout;
    
    User(int uid, double think_mean, double think_std, double t);
    void issue_req();
    void handle_reply(int req_id) ;
    void handle_timeout(int req_id);
};

class Server {
    public :
    int n_cores;
    int tot_threads;
    int free_threads;
    int queue_capacity;
    double mean_service_time;
    double time_slice;
    
    queue<int> req_queue;
    queue<int> thread_queue;
    vector<int> core_status;
    
    Server(int cores, int threads, int q_cap, double service_mean, double slice);
    void assign_core();                                         // assigns cores to the threads in the front of the thread queue;
    void handle_arrival(int req_id);                            
    void assign_thread();                                      // assigns a thread to the request in the front of the quue and pushes it to the therad queue;
    void handle_context_switch(int req_id, int core_id);
    void handle_departure(int req_id, int core_id);
};

int num_in_system(Server &server);