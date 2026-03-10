#include "simulator.hh"
#include <fstream>

double compute_avg_response_time() {

    double total_response_time = 0.0;
    int count = 0;

    for (const Request &req : requests) {
        if (!req.dropped && !req.timed_out && req.completion_time != -1) {
            double response = req.completion_time - req.issue_time;
            total_response_time += response;
            count++;
        }
    }

    if (count == 0) return 0.0;
    return total_response_time / count;
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

void simulate(Config &config) {
    int NUM_USERS = config.num_users;
    double THINK_TIME_MEAN = config.think_time_mean;
    double THINK_TIME_STD = config.think_time_std;
    double TIMEOUT = config.timeout;
    int NCORES = config.num_cores;
    int TOT_THREADS = config.tot_threads;
    int QUEUE_CAP = config.queue_capacity;
    double SERVICE_TIME_MEAN = config.service_time_mean;
    double TIME_SLICE = config.quantum_time_slice;

    vector<User> users;
    for (int i = 0; i < NUM_USERS; i++) {
        User u = User(i,THINK_TIME_MEAN, THINK_TIME_STD, TIMEOUT);
        users.push_back(u); 
    }

    Server server(NCORES, TOT_THREADS, QUEUE_CAP, SERVICE_TIME_MEAN, TIME_SLICE);

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
    current_time = 0;
    requests.clear();
    while (!event_heap.empty()) event_heap.pop();
    return;
}

int main() {

    Config config;
    config.think_time_mean = 5.0;
    config.think_time_std = 0.5;
    config.timeout = 20.0;
    config.num_cores = 4;
    config.tot_threads = 4;
    config.queue_capacity = 50;
    config.service_time_mean = 0.5;
    config.quantum_time_slice = 100;

    int RUNS = 20;   

    ofstream outfile("response_time_vs_users_ci.csv");
    outfile << "users,mean_rt,lower_ci,upper_ci\n";

    for (int users = 5; users <= 100; users += 5) {

        vector<double> samples;

        for (int r = 0; r < RUNS; r++) {

            config.num_users = users;

            simulate(config);

            samples.push_back(compute_avg_response_time());

            reset();
        }

        double m = mean(samples);
        double sd = stddev(samples, m);

        double ci = 1.96 * sd / sqrt(RUNS);  // 95% confidence interval

        double lower = m - ci;
        double upper = m + ci;

        outfile << users << "," << m << "," << lower << "," << upper << "\n";
    }

    outfile.close();
}

    // cout << "Simulation Finished\n";
    // cout << "Completed Requests: " << completed_requests << endl;
    // cout << "Total Requests Generated: " << global_request_counter << endl;
    // cout << "Average Response Time : " << compute_avg_response_time() << endl;

