#include <bits/stdc++.h>
using namespace std;

enum EventType {
    REQUEST_ARRIVAL = 1,
    REQUEST_DEPARTURE = 2,
    REQUEST_TIMEOUT = 3,
    CONTEXT_SWITCH = 4
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

    Event(double t, int type, int req = -1, int core = -1) ;
    bool operator<(const Event& other) const;
};

//global variables;
priority_queue<Event> event_heap;
vector<Request> requests;          // store all requests for further statistics calculation;
int global_request_counter = 0;
int MAX_REQUESTS = 1000;
int completed_requests = 0;
double current_time = 0;
default_random_engine generator;

//distribution functions;
double gaussian_sample(double mean, double stddev) {
    normal_distribution<double> dist(mean, stddev);
    return max(0.0, dist(generator)); 
}

double exponential_sample(double mean) {
    exponential_distribution<double> dist(1.0 / mean);
    return dist(generator);
}

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