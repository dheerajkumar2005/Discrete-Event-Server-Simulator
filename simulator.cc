#include "simulator.hh"

priority_queue<Event> event_heap; // event manager
vector<Request> requests;
int global_request_counter = 0;
int MAX_REQUESTS = 15000; // max requests before completion
int completed_requests = 0;
double current_time = 0; // global time tracker
int WARMUP_REQUESTS = 3000; // Ignore these many requests for calculation of metrics 
int measured_completions = 0;
int good_completions = 0;
int bad_completions = 0;
double warmup_end_time = -1;
double total_core_busy_time = 0;
int measured_arrivals = 0;
int dropped_requests = 0;
double last_event_time = 0;
double area_num_system = 0; // integral n*dt
double area_queue_length = 0; // integral q*dt
Sampler s;

// default_random_engine generator;

#include <iostream>
#include <variant>
#include <random>
#include <string>

Sampler::Sampler() : rng(std::random_device{}()) {}

double Sampler::sample(const Distribution& dist) {
    return std::visit([&](auto&& d) -> double {
        using T = std::decay_t<decltype(d)>;

        if constexpr (std::is_same_v<T, Normal>) {
            return std::normal_distribution<>{d.mean, d.stddev}(rng);
        } else if constexpr (std::is_same_v<T, Uniform>) {
            return std::uniform_real_distribution<>{d.low, d.high}(rng);
        } else if constexpr (std::is_same_v<T, Constant>) {
            return d.value;
        } else if constexpr (std::is_same_v<T, Exponential>) {
            return std::exponential_distribution<>{d.lambda}(rng);
        }
    }, dist);
}

std::string Sampler::name(const Distribution& dist) {
    return std::visit([](auto&& d) -> std::string {
        using T = std::decay_t<decltype(d)>;
        if constexpr (std::is_same_v<T, Normal>)      return "Normal";
        if constexpr (std::is_same_v<T, Uniform>)     return "Uniform";
        if constexpr (std::is_same_v<T, Constant>)   return "Constant";
        if constexpr (std::is_same_v<T, Exponential>) return "Exponential";
    }, dist);
}


Distribution parse_distribution(const string& value) {
    istringstream ss(value);
    string type;
    ss >> type;

    if (type == "normal") {
        double mean, stddev;
        ss >> mean >> stddev;
        return Normal{mean, stddev};
    } else if (type == "uniform") {
        double low, high;
        ss >> low >> high;
        return Uniform{low, high};
    } else if (type == "exponential") {
        double lambda;
        ss >> lambda;
        return Exponential{lambda};
    } else if (type == "constant") {
        double val;
        ss >> val;
        return Constant{val};
    } else {
        cerr << "Unknown distribution type: " << type << "\n";
        exit(1);
    }
}

Config::Config(){
    think_time = Normal{5,0.5};
    timeout = Normal{5,0.5};
    service_time = Exponential{0.05};
    num_cores = 1;
    tot_threads = 1;
    queue_capacity = 100000;
    quantum_time_slice = 100000;
    context_switch_overhead = 0;
    min_users = 20;
    max_users = 200;
    user_step_size = 10;
    runs = 30;
}

void Config::load_config(const string& filename) {
    ifstream file(filename);
    if (!file) {
        cerr << "Could not open config file\n";
        exit(1);
    }

    string line;
    while (getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        if (line.empty()) continue;
        if (line.rfind("//", 0) == 0) continue;

        int pos = line.find('=');
        if (pos == string::npos) continue;

        string key = line.substr(0, pos);
        string value = line.substr(pos + 1);

        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        if      (key == "think_time")             this->think_time   = parse_distribution(value);
        else if (key == "timeout")                this->timeout      = parse_distribution(value);
        else if (key == "service_time")           this->service_time = parse_distribution(value);
        else if (key == "num_cores")              this->num_cores              = stoi(value);
        else if (key == "tot_threads")            this->tot_threads            = stoi(value);
        else if (key == "queue_capacity")         this->queue_capacity         = stoi(value);
        else if (key == "quantum_time_slice")     this->quantum_time_slice     = stod(value);
        else if (key == "context_switch_overhead")this->context_switch_overhead= stod(value);
        else if (key == "min_users")              this->min_users              = stoi(value);
        else if (key == "max_users")              this->max_users              = stoi(value);
        else if (key == "user_step_size")         this->user_step_size         = stoi(value);
        else if (key == "runs")                   this->runs                   = stoi(value);
    }
}



int num_in_system(Server &server) {
    int running = 0;

    for (int x : server.core_status)
        if (x != -1) running++;

    return server.req_queue.size() + server.thread_queue.size() + running;
}

Request::Request(int r_id, int u_id, double issue_time) {
    req_id = r_id;
    user_id = u_id;
    issue_time = issue_time;
    thread_assigned_time = -1;
    completion_time = -1;
    total_service_time = 0;
    service_time_completed = 0;
    timeout_time = 0;
    timed_out = false;
    dropped = false;
    context_switches = 0;
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


//user functions;
User::User(int uid, Distribution think_time, Distribution timeout) {
    user_id = uid;
    this->think_time = think_time;
    this->timeout = timeout;
}

void User::issue_req() {
    double think = max(0.0, s.sample(think_time));
    double issue_time = current_time + think;
    int req_id = global_request_counter++;
    Request req(req_id, user_id, issue_time);
    req.timeout_time = issue_time + s.sample(timeout);

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
    issue_req();
    
}

//server functions;
Server::Server(int cores, int threads, int q_cap, Distribution service_time, double slice, double overhead) {

    n_cores = cores;
    tot_threads = threads;
    free_threads = threads;
    queue_capacity = q_cap;
    this->service_time = service_time;
    time_slice = slice;
    core_status.resize(cores, -1);
    context_switch_overhead = overhead;

}

void Server::assign_core()  {
    // check for a free core and assign it a thread from thread queue;
    
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
            event_heap.push(Event(current_time + time_slice + context_switch_overhead, CONTEXT_SWITCH, req_id, i));
        }
    }
}

void Server::handle_arrival(int req_id) {
    // Check if queue is full, if not check if there is a free thread if not wait in the queue else assign the request to a thread;
    Request &req = requests[req_id];
    if (warmup_end_time >= 0 && current_time >= warmup_end_time) measured_arrivals++;
    int k = req_queue.size();

    if (k == queue_capacity) {
        //queue full, drop request;
        req.dropped = true;
        if (warmup_end_time >= 0 && current_time >= warmup_end_time) dropped_requests++;
        return;
    }

    if (free_threads > 0) {
        //assign request a thread;
        free_threads--;
        req.thread_assigned_time = current_time;
        req.total_service_time = s.sample(service_time);
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
        req.total_service_time = s.sample(service_time);
        req.service_time_completed = 0;
        thread_queue.push(req_id);
    }
}

void Server::handle_context_switch(int req_id, int core_id) {

    Request &req = requests[req_id];
    req.service_time_completed += time_slice;
    req.context_switches++;
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

Metrics::Metrics() {}

double Metrics::compute_avg_response_time() {

    double total_response_time = 0.0;
    int count = 0;

    for (const Request &req : requests) {
        if (!req.dropped && !req.timed_out && req.completion_time != -1 && req.completion_time > warmup_end_time) {
            double response = req.completion_time - req.issue_time;
            total_response_time += response;
            count++;
        }
    }

    if (count == 0) return 0.0;
    return total_response_time / count;
}

double Metrics::compute_avg_context_switches() {
    int total_context_switches = 0;
    int count = 0;
    for (const Request &req : requests) {
        if (!req.dropped && !req.timed_out && req.completion_time != -1 && req.completion_time > warmup_end_time) {
            total_context_switches += req.context_switches;
            count++;
        }
    }
    if(count == 0) return 0.0;
    return double(total_context_switches)/double(count);
}


double mean(vector<double> &v) {
    double s = 0;
    for (double x : v) s += x;
    return s / v.size();
}

double stddev(vector<double> &v, double m) {
    double s = 0;
    for (double x : v) s += (x - m) * (x - m);
    return sqrt(s / (v.size() - 1));
}


Metrics compute_metrics(Config &config) {
    Metrics m;
    double measurement_time = current_time - warmup_end_time;

    m.avg_response = m.compute_avg_response_time();
    m.throughput = measured_completions / measurement_time;
    m.goodput = good_completions / measurement_time;
    m.badput = bad_completions / measurement_time;
    m.utilization = total_core_busy_time / (config.num_cores * measurement_time);
    m.avg_drop_rate = (double)dropped_requests / measured_arrivals;
    m.avg_num_system = area_num_system / measurement_time;
    m.avg_queue_length = area_queue_length / measurement_time;
    m.avg_context_switches = m.compute_avg_context_switches();

    return m;
}



void simulate(Config &config, int num_users) {
    int NUM_USERS = num_users;
    Distribution THINK_TIME = config.think_time;
    Distribution TIMEOUT = config.timeout;
    int NCORES = config.num_cores;
    int TOT_THREADS = config.tot_threads;
    int QUEUE_CAP = config.queue_capacity;
    Distribution SERVICE_TIME = config.service_time;
    double TIME_SLICE = config.quantum_time_slice;
    double OVERHEAD = config.context_switch_overhead;
    vector<User> users;
    for (int i = 0; i < NUM_USERS; i++) {
        User u = User(i,THINK_TIME, TIMEOUT);
        users.push_back(u); 
    }

    Server server(NCORES, TOT_THREADS, QUEUE_CAP, SERVICE_TIME, TIME_SLICE, OVERHEAD);

    //send initial requests;
    for (auto &u : users) {
        u.issue_req();
    }

    //main simulation loop;
    while (!event_heap.empty() && completed_requests < MAX_REQUESTS) {   //decide a better stopping criterion;

        Event ev = event_heap.top();
        event_heap.pop();

        current_time = ev.time;
        int req_id = ev.request_id;
        double dt = current_time - last_event_time;

        if (warmup_end_time >= 0) {
            int nsys = num_in_system(server);
            int qlen = server.req_queue.size();

            area_num_system += nsys * dt;
            area_queue_length += qlen * dt;
        }

        last_event_time = current_time;

        switch (ev.event_type) {

            case REQUEST_ARRIVAL:
                server.handle_arrival(req_id);
                break;

            case REQUEST_DEPARTURE: {
                server.handle_departure(req_id, ev.core_id);
                Request &req = requests[req_id];
                users[req.user_id].handle_reply(req_id);
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
                server.handle_context_switch(req_id, ev.core_id);
                break;

            default:
                cerr << "Unknown event type\n";
                exit(1);
        }
    }
}


void reset() {
    global_request_counter = 0;
    completed_requests = 0;
    measured_completions = 0;
    good_completions = 0;
    bad_completions = 0;
    warmup_end_time = -1;
    total_core_busy_time = 0;
    measured_arrivals = 0;
    dropped_requests = 0;
    current_time = 0;
    area_num_system = 0;
    area_queue_length = 0;
    last_event_time = 0;
    requests.clear();
    while (!event_heap.empty()) event_heap.pop();
    return;
}