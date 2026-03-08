#include "simulator.hh"

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

Event::Event(double t, int type, int req = -1, int core = -1) {
    time = t;
    event_type = type;
    request_id = req;
    core_id = core;
}
bool Event::operator<(const Event& other) const {
    return time > other.time;
}

//user functions;
User::User(int uid, double think_mean, double think_std, double t) {
    user_id = uid;
    think_time_mean = think_mean;
    think_time_std = think_std;
    timeout = t;
}

void User::issue_req() {

    double think = gaussian_sample(think_time_mean, think_time_std);
    double issue_time = current_time + think;
    int req_id = global_request_counter++;
    Request req(req_id, user_id, issue_time);
    req.timeout_time = current_time + timeout;

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
        completed_requests++;
        return;
    }
    // upon recieving a valid reply issue request again;
    issue_req();

}

void User::handle_timeout(int req_id) {

    Request &req = requests[req_id];
    if (req.completion_time != -1) {
        // if the request is already finished do nothing. We can handle this in the driver also so this check is a little redundant;
        return;
    }
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
    if (thread_queue.empty()) return;

    for (int i=0; i < n_cores; i++) {

        if (core_status[i] != -1) continue;
        int req_id = thread_queue.front();
        thread_queue.pop();
        Request &req = requests[req_id];
        core_status[i] = req_id;

        double remaining_service_time = req.total_service_time - req.service_time_completed;

        if (remaining_service_time <= time_slice) {
            // push departure event;
            event_heap.push(Event(current_time + remaining_service_time, REQUEST_DEPARTURE, req_id, i));
        }
        else {
            // push context switch event;
            event_heap.push(Event(current_time + remaining_service_time, CONTEXT_SWITCH, req_id, i));
        }
    }
}

void Server::handle_arrival(int req_id) {
    // Check if queue is full, if not ccheck if there is a free thread if not wait in the queue else assign the request to a thread;
    Request &req = requests[req_id];
    int k = req_queue.size();

    if (k >= queue_capacity) {
        //queue full, drop request;
        req.dropped = true;
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
        req_queue.push(req_id);
    }
}

void Server::handle_context_switch(int req_id, int core_id) {

    Request &req = requests[req_id];
    req.service_time_completed += time_slice;
    req_queue.push(req_id);
    core_status[core_id] = -1;
    assign_core();

}

void Server::handle_departure(int req_id, int core_id) {

    Request &req = requests[req_id];
    req.completion_time = current_time;
    core_status[core_id] = -1;
    free_threads++;
    completed_requests++;
    assign_thread();
    assign_core();
    
}