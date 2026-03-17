#include "tandem_simulator.hh"
#include <fstream>

double compute_avg_response_time() {

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

    m.avg_response = compute_avg_response_time();
    m.throughput = measured_completions / measurement_time;
    m.goodput = good_completions / measurement_time;
    m.badput = bad_completions / measurement_time;
    m.util1 = total_core_busy_time_s1 / (config.num_cores * measurement_time);
    m.util2 = total_core_busy_time_s2 / (config.num_cores * measurement_time);
    m.drop_rate = (double)dropped_reqests / measured_arrivals;
    m.avg_num_system = area_num_system / measurement_time;
    m.avg_queue1_length = area_queue1_length / measurement_time;
    m.avg_queue2_length = area_queue2_length / measurement_time;

    return m;
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

    Server server1(NCORES, TOT_THREADS, QUEUE_CAP, SERVICE_TIME_MEAN, TIME_SLICE);
    Server server2(NCORES, TOT_THREADS, QUEUE_CAP, SERVICE_TIME_MEAN, TIME_SLICE);

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

            int nsys = num_in_system(server1) + num_in_system(server2);

            int qlen1 = server1.req_queue.size();
            int qlen2 = server2.req_queue.size();

            area_num_system += nsys * dt;

            area_queue1_length += qlen1 * dt;
            area_queue2_length += qlen2 * dt;
        }

        last_event_time = current_time;

        switch (ev.event_type) {

            case REQUEST_ARRIVAL:
                if(ev.server_id == 0)
                    server1.handle_arrival(req_id);
                else
                    server2.handle_arrival(req_id);
                break;

            case REQUEST_DEPARTURE: {

                Request &req = requests[req_id];
                if (ev.server_id == 0) {
                    server1.handle_departure(req_id, ev.core_id);  // take the request out of server1
                    double r = uniform_real_distribution<>(0,1)(generator);

                    if (r < config.routing_prob) {  // move to server 2
                        req.current_server = 1;  
                        req.total_service_time = 0;  // reset service parameters
                        req.service_time_completed = 0;
                        event_heap.push(Event(current_time, REQUEST_ARRIVAL,req_id,-1,1));
                    } 
                    else {  // move back to user

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
                }

                else {  // request fininshed in server 2

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

                if(ev.server_id == 0)
                    server1.handle_context_switch(req_id, ev.core_id);
                else
                    server2.handle_context_switch(req_id, ev.core_id);

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
    total_core_busy_time_s1 = 0;
    total_core_busy_time_s2 = 0;
    measured_arrivals = 0;
    dropped_reqests = 0;
    current_time = 0;
    area_num_system = 0;
    area_queue1_length = 0;
    area_queue2_length = 0;
    last_event_time = 0;
    requests.clear();
    while (!event_heap.empty()) event_heap.pop();
    return;
}

void load_config(string filename, Config &config, int &RUNS, int &user_start, int &user_end, int &user_step) {

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

        double val = stod(value);

        if (key == "think_time_mean") config.think_time_mean = val;
        else if (key == "think_time_std") config.think_time_std = val;
        else if (key == "timeout") config.timeout = val;
        else if (key == "num_cores") config.num_cores = val;
        else if (key == "tot_threads") config.tot_threads = val;
        else if (key == "queue_capacity") config.queue_capacity = val;
        else if (key == "service_time_mean") config.service_time_mean = val;
        else if (key == "quantum_time_slice") config.quantum_time_slice = val;
        else if (key == "runs") RUNS = val;
        else if (key == "user_start") user_start = val;
        else if (key == "user_end") user_end = val;
        else if (key == "user_step") user_step = val;
        else if (key == "routing_probability") config.routing_prob = val;
    }
}

int main() {

    Config config;
    int RUNS;
    int user_start, user_end, user_step;

    load_config("config.txt", config, RUNS, user_start, user_end, user_step);
    ofstream outfile("metrics.csv");
    outfile << "users,mean_rt,lower_ci,upper_ci,throughput,goodput,badput,util_server1,util_server2,drop_rate,avg_num_system,avg_queue_length_s1,avg_queue_length_s2\n";

    for (int users = user_start; users <= user_end; users += user_step) {
        vector<double> samples;
        double throughput_sum = 0;
        double goodput_sum = 0;
        double badput_sum = 0;
        double util1_sum = 0;
        double util2_sum = 0;
        double drop_sum = 0;
        double nsys_sum = 0;
        double qlen1_sum = 0;
        double qlen2_sum = 0;

        for (int r = 0; r < RUNS; r++) {
            config.num_users = users;
            simulate(config);

            samples.push_back(compute_avg_response_time());
            Metrics m = compute_metrics(config);
            throughput_sum += m.throughput;
            goodput_sum += m.goodput;
            badput_sum += m.badput;
            util1_sum += m.util1;
            util2_sum += m.util2;
            drop_sum += m.drop_rate;
            nsys_sum += m.avg_num_system;
            qlen1_sum += m.avg_queue1_length;
            qlen2_sum += m.avg_queue2_length;

            reset();
        }

        double m = mean(samples);
        double sd = stddev(samples, m);
        double ci = 1.96 * sd / sqrt(RUNS);  // 95% confidence interval
        double lower = m - ci;
        double upper = m + ci;

        double throughput = throughput_sum / RUNS;
        double goodput = goodput_sum / RUNS;
        double badput = badput_sum / RUNS;
        double util1 = util1_sum / RUNS;
        double util2 = util2_sum / RUNS;
        double drop_rate = drop_sum / RUNS;
        double avg_nsys = nsys_sum / RUNS;
        double avg_qlen1 = qlen1_sum / RUNS;
        double avg_qlen2 = qlen2_sum / RUNS;

        outfile << users << "," << m << "," << lower << "," << upper << "," << throughput << "," << goodput << "," << badput << "," << util1 << "," << util2 << "," << drop_rate << "," << avg_nsys << "," << avg_qlen1 << "," << avg_qlen2 << "\n";
    }

    outfile.close();
}

    // cout << "Simulation Finished\n";
    // cout << "Completed Requests: " << completed_requests << endl;
    // cout << "Total Requests Generated: " << global_request_counter << endl;
    // cout << "Average Response Time : " << compute_avg_response_time() << endl;

