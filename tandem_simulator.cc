#include "tandem_simulator.hh"

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
double total_core_busy_time_s1 = 0;
double total_core_busy_time_s2 = 0;
int measured_arrivals = 0;
int dropped_reqests = 0;
double last_event_time = 0;
double area_num_system = 0;
double area_queue1_length = 0;
double area_queue2_length = 0;
default_random_engine generator;

int num_in_system(Server &server) {
    int running = 0;
    for (int x : server.core_status)
        if (x != -1) running++;
    return server.req_queue.size() + server.thread_queue.size() + running;
}

Request::Request(int r_id, int u_id, double issue, int cur_ser) {
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
    current_server = cur_ser;
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

User::User(int uid, double think_mean, double think_std, double t) {
    user_id = uid;
    think_time_mean = think_mean;
    think_time_std = think_std;
    timeout = t;
}

void User::issue_req() {
    double think = max(0.0, gaussian_sample(think_time_mean, think_time_std));
    double issue_time = current_time + think;

    int req_id = global_request_counter++;
    Request req(req_id, user_id, issue_time);
    req.timeout_time = issue_time + timeout;

    requests.push_back(req);

    event_heap.push(Event(issue_time, REQUEST_ARRIVAL, req_id, -1, 0));
    event_heap.push(Event(req.timeout_time, REQUEST_TIMEOUT, req_id));
}

void User::handle_reply(int req_id) {
    Request &req = requests[req_id];
    if (!req.timed_out)
        issue_req();
}

void User::handle_timeout(int req_id) {
    issue_req();
}

Server::Server(int cores, int threads, int q_cap, double service_mean, double slice) {
    n_cores = cores;
    tot_threads = threads;
    free_threads = threads;
    queue_capacity = q_cap;
    mean_service_time = service_mean;
    time_slice = slice;
    core_status.resize(cores, -1);
}

void Server::assign_core() {
    for (int i = 0; i < n_cores; i++) {
        if (core_status[i] != -1) continue;
        if (thread_queue.empty()) return;

        int req_id = thread_queue.front();
        thread_queue.pop();
        Request &req = requests[req_id];

        if (req.timed_out || req.dropped) continue;

        core_status[i] = req_id;

        double remaining = req.total_service_time - req.service_time_completed;
        double run_time = min(remaining, time_slice);

        if (completed_requests >= WARMUP_REQUESTS) {
            if (req.current_server == 0)
                total_core_busy_time_s1 += run_time;
            else
                total_core_busy_time_s2 += run_time;
        }

        if (remaining <= time_slice)
            event_heap.push(Event(current_time + remaining, REQUEST_DEPARTURE, req_id, i, req.current_server));
        else
            event_heap.push(Event(current_time + time_slice, CONTEXT_SWITCH, req_id, i, req.current_server));
    }
}

void Server::handle_arrival(int req_id) {
    Request &req = requests[req_id];

    if (warmup_end_time >= 0)
        measured_arrivals++;

    if ((int)req_queue.size() >= queue_capacity) {
        req.dropped = true;
        if (warmup_end_time >= 0) dropped_reqests++;
        return;
    }

    if (free_threads > 0) {
        free_threads--;
        req.thread_assigned_time = current_time;
        req.total_service_time = exponential_sample(mean_service_time);
        req.service_time_completed = 0;
        thread_queue.push(req_id);
        assign_core();
    } else {
        req_queue.push(req_id);
    }
}

void Server::assign_thread() {
    while (free_threads > 0 && !req_queue.empty()) {
        int req_id = req_queue.front();
        req_queue.pop();

        Request &req = requests[req_id];
        if (req.timed_out || req.dropped) continue;

        free_threads--;
        req.thread_assigned_time = current_time;

        if (req.total_service_time == 0) {
            req.total_service_time = exponential_sample(mean_service_time);
            req.service_time_completed = 0;
        }

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
    core_status[core_id] = -1;
    free_threads++;
    assign_thread();
    assign_core();
}


void simulate(Config &config) {

    int NUM_USERS = config.num_users;
    double THINK_TIME_MEAN = config.think_time_mean;
    double THINK_TIME_STD = config.think_time_std;
    double TIMEOUT = config.timeout;

    vector<User> users;
    for (int i = 0; i < NUM_USERS; i++) {
        users.emplace_back(i, THINK_TIME_MEAN, THINK_TIME_STD, TIMEOUT);
    }

    Server server1(
        config.num_cores_s1,
        config.tot_threads_s1,
        config.queue_capacity_s1,
        config.service_time_mean_s1,
        config.quantum_time_slice_s1
    );

    Server server2(
        config.num_cores_s2,
        config.tot_threads_s2,
        config.queue_capacity_s2,
        config.service_time_mean_s2,
        config.quantum_time_slice_s2
    );

    // initial requests
    for (auto &u : users)
        u.issue_req();

    while (!event_heap.empty() && completed_requests < MAX_REQUESTS) {

        Event ev = event_heap.top();
        event_heap.pop();

        current_time = ev.time;
        int req_id = ev.request_id;
        double dt = current_time - last_event_time;

        if (warmup_end_time >= 0) {
            int nsys = num_in_system(server1) + num_in_system(server2);
            area_num_system += nsys * dt;
            area_queue1_length += server1.req_queue.size() * dt;
            area_queue2_length += server2.req_queue.size() * dt;
        }

        last_event_time = current_time;

        switch (ev.event_type) {

            case REQUEST_ARRIVAL:
                if (ev.server_id == 0)
                    server1.handle_arrival(req_id);
                else
                    server2.handle_arrival(req_id);
                break;

            case REQUEST_DEPARTURE: {
                Request &req = requests[req_id];

                if (ev.server_id == 0) {
                    server1.handle_departure(req_id, ev.core_id);

                    double r = uniform_real_distribution<>(0,1)(generator);

                    if (r < config.routing_prob) {
                        req.current_server = 1;
                        req.total_service_time = 0;
                        req.service_time_completed = 0;
                        event_heap.push(Event(current_time, REQUEST_ARRIVAL, req_id, -1, 1));
                    } else {
                        req.completion_time = current_time;
                        completed_requests++;

                        if (completed_requests == WARMUP_REQUESTS)
                            warmup_end_time = current_time;

                        if (completed_requests > WARMUP_REQUESTS) {
                            measured_completions++;
                            if (req.timed_out) bad_completions++;
                            else good_completions++;
                        }

                        users[req.user_id].handle_reply(req_id);
                    }
                } else {
                    server2.handle_departure(req_id, ev.core_id);

                    req.completion_time = current_time;
                    completed_requests++;

                    if (completed_requests == WARMUP_REQUESTS)
                        warmup_end_time = current_time;

                    if (completed_requests > WARMUP_REQUESTS) {
                        measured_completions++;
                        if (req.timed_out) bad_completions++;
                        else good_completions++;
                    }

                    users[req.user_id].handle_reply(req_id);
                }

                break;
            }

            case REQUEST_TIMEOUT: {
                Request &req = requests[req_id];
                if (req.completion_time == -1) {
                    req.timed_out = true;
                    users[req.user_id].handle_timeout(req_id);
                }
                break;
            }

            case CONTEXT_SWITCH:
                if (ev.server_id == 0)
                    server1.handle_context_switch(req_id, ev.core_id);
                else
                    server2.handle_context_switch(req_id, ev.core_id);
                break;

            default:
                cerr << "Unknown event\n";
                exit(1);
        }
    }
}