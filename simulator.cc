#include "simulator.hh"

priority_queue<Event> event_heap;
vector<Request> requests;
int global_request_counter = 0;
int MAX_REQUESTS = 15000;
int completed_requests = 0;
double current_time = 0;
int WARMUP_REQUESTS = 3000;
int measured_completions = 0;
int good_completions = 0;
int bad_completions = 0;
double warmup_end_time = -1;
double total_core_busy_time = 0;
int measured_arrivals = 0;
int dropped_requests = 0;
double last_event_time = 0;
double area_num_system = 0;
double area_queue_length = 0;
default_random_engine generator;

int num_in_system(Server &server) {
    int running = 0;

    for (int x : server.core_status)
        if (x != -1) running++;

    return server.req_queue.size() + server.thread_queue.size() + running;
}

Config::Config() {}
Metrics::Metrics() {}

Request::Request(int r_id, int u_id, double issue) {
    req_id = r_id;
    user_id = u_id;
    issue_time = issue;
    thread_assigned_time = -1;
    completion_time = -1;
    total_service_time = 0;
    service_time_completed = 0;
    timeout_time = 0;
    timed_out = false;
    dropped = false;
}

Event::Event(double t, int type, int req, int core, int ser) {
    time = t;
    event_type = type;
    request_id = req;
    core_id = core;
    server_id = ser;
}
bool Event::operator<(const Event& other) const {
    return time > other.time;
}

double gaussian_sample(double mean, double stddev) {
    if (stddev == 0) return mean;
    normal_distribution<double> dist(mean, stddev);
    return max(0.0, dist(generator)); 
}

double exponential_sample(double mean) {
    exponential_distribution<double> dist(1.0 / mean);
    return dist(generator);
}

//user functions;
User::User(int uid, double think_mean, double think_std, double t) {
    user_id = uid;
    think_time_mean = think_mean;
    think_time_std = think_std;
    timeout = t;
}

void User::issue_req() {

    double think = max(0.0,gaussian_sample(think_time_mean, think_time_std));
    double issue_time = current_time + think;
    int req_id = global_request_counter++;
    Request req(req_id, user_id, issue_time);
    req.timeout_time = issue_time + timeout + exponential_sample(1.0);

    requests.push_back(req);
    event_heap.push(Event(issue_time, REQUEST_ARRIVAL, req_id));  // push arrival event;
    event_heap.push(Event(req.timeout_time, REQUEST_TIMEOUT, req_id));  // push timeout event;

}

void User::handle_reply(int req_id) {

    Request &req = requests[req_id];
    if (req.user_id != user_id) {
        cerr << "This Shoudn't happen hopefully !\n";
        exit(0);
    }
    if (req.timed_out) {
        // if we get the reply of a timedout request, do nothing;
        return;
    }
    // upon recieving a valid reply issue request again;
    issue_req();

}

void User::handle_timeout(int req_id) {

    Request &req = requests[req_id];
    issue_req();
    
}

//server functions;
Server::Server(int cores, int threads, int q_cap, double service_mean, double slice) {

    n_cores = cores;
    tot_threads = threads;
    free_threads = threads;
    queue_capacity = q_cap;
    mean_service_time = service_mean;
    time_slice = slice;
    core_status.resize(cores, -1);

}

void Server::assign_core()  {
    // checck for a free core and assign it a thread from thread queue;
    
    for (int i=0; i < n_cores; i++) {

        if (core_status[i] != -1) continue;
        if (thread_queue.empty()) return;
        int req_id = thread_queue.front();
        thread_queue.pop();
        Request &req = requests[req_id];
        core_status[i] = req_id;

        double remaining_service_time = req.total_service_time - req.service_time_completed;
        double run_time = min(remaining_service_time, time_slice);
        if (completed_requests >= WARMUP_REQUESTS) total_core_busy_time += run_time;

        if (remaining_service_time <= time_slice) {
            // push departure event;
            event_heap.push(Event(current_time + remaining_service_time, REQUEST_DEPARTURE, req_id, i));
        }
        else {
            // push context switch event;
            event_heap.push(Event(current_time + time_slice, CONTEXT_SWITCH, req_id, i));
        }
    }
}

void Server::handle_arrival(int req_id) {
    // Check if queue is full, if not ccheck if there is a free thread if not wait in the queue else assign the request to a thread;
    Request &req = requests[req_id];
    if (warmup_end_time >= 0 && current_time >= warmup_end_time) measured_arrivals++;
    int k = req_queue.size();

    if (k >= queue_capacity) {
        //queue full, drop request;
        req.dropped = true;
        if (warmup_end_time >= 0 && current_time >= warmup_end_time) dropped_requests++;
        return;
    }

    if (free_threads > 0) {
        //assign request a thread;
        free_threads--;
        req.thread_assigned_time = current_time;
        req.total_service_time = exponential_sample(mean_service_time);
        req.service_time_completed = 0;
        thread_queue.push(req_id);
        assign_core();
    }
    else {
        req_queue.push(req_id);
    }
}

void Server::assign_thread() {
    // assigns a thread to a request in the front of the queue;
    // This function is called when a request departs beacuse that is the only time a thread is freed;
    while (free_threads > 0 && !req_queue.empty()) { // there should only be one free thread at here but just for safety, put a while loop
        int req_id = req_queue.front();
        req_queue.pop();
        Request &req = requests[req_id];
        free_threads--;
        req.thread_assigned_time = current_time;
        req.total_service_time = exponential_sample(mean_service_time);
        req.service_time_completed = 0;
        thread_queue.push(req_id);
    }
}

void Server::handle_context_switch(int req_id, int core_id) {

    Request &req = requests[req_id];
    req.service_time_completed += time_slice;
    thread_queue.push(req_id);
    core_status[core_id] = -1;
    assign_core();

}

void Server::handle_departure(int req_id, int core_id) {

    Request &req = requests[req_id];
    req.completion_time = current_time;
    core_status[core_id] = -1;
    free_threads++;
    completed_requests++;

    if (completed_requests == WARMUP_REQUESTS) warmup_end_time = current_time;
    
    if (completed_requests > WARMUP_REQUESTS) {
        measured_completions++;
        if (req.timed_out) bad_completions++;
        else good_completions++;
    }
    assign_thread();
    assign_core();
    
}