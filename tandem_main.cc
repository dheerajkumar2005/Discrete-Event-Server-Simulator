#include "tandem_simulator.hh"
#include <fstream>

double compute_avg_response_time() {
    double total = 0;
    int count = 0;

    for (const Request &req : requests) {
        if (!req.dropped && !req.timed_out && req.completion_time > warmup_end_time) {
            total += (req.completion_time - req.issue_time);
            count++;
        }
    }
    return count ? total / count : 0;
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
    double T = current_time - warmup_end_time;

    m.avg_response = compute_avg_response_time();
    m.throughput = measured_completions / T;
    m.goodput = good_completions / T;
    m.badput = bad_completions / T;
    m.util1 = total_core_busy_time_s1 / (config.num_cores_s1 * T);
    m.util2 = total_core_busy_time_s2 / (config.num_cores_s2 * T);
    m.drop_rate = (double)dropped_reqests / measured_arrivals;
    m.avg_num_system = area_num_system / T;
    m.avg_queue1_length = area_queue1_length / T;
    m.avg_queue2_length = area_queue2_length / T;

    return m;
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
        if (line.empty() || line.rfind("//", 0) == 0) continue;

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

        else if (key == "num_cores_s1") config.num_cores_s1 = val;
        else if (key == "tot_threads_s1") config.tot_threads_s1 = val;
        else if (key == "queue_capacity_s1") config.queue_capacity_s1 = val;
        else if (key == "service_time_mean_s1") config.service_time_mean_s1 = val;
        else if (key == "quantum_time_slice_s1") config.quantum_time_slice_s1 = val;

        else if (key == "num_cores_s2") config.num_cores_s2 = val;
        else if (key == "tot_threads_s2") config.tot_threads_s2 = val;
        else if (key == "queue_capacity_s2") config.queue_capacity_s2 = val;
        else if (key == "service_time_mean_s2") config.service_time_mean_s2 = val;
        else if (key == "quantum_time_slice_s2") config.quantum_time_slice_s2 = val;

        else if (key == "routing_probability") config.routing_prob = val;

        else if (key == "runs") RUNS = val;
        else if (key == "user_start") user_start = val;
        else if (key == "user_end") user_end = val;
        else if (key == "user_step") user_step = val;
    }
}

int main() {
    generator.seed(chrono::system_clock::now().time_since_epoch().count());

    Config config;
    int RUNS, user_start, user_end, user_step;

    load_config("tandem_config.txt", config, RUNS, user_start, user_end, user_step);

    ofstream outfile("metrics.csv");
    outfile << "users,mean_rt,lower_ci,upper_ci,throughput,goodput,badput,util_server1,util_server2,drop_rate,avg_num_system,avg_queue_length_s1,avg_queue_length_s2\n";

    for (int users = user_start; users <= user_end; users += user_step) {

        vector<double> samples;
        double throughput_sum = 0, goodput_sum = 0, badput_sum = 0;
        double util1_sum = 0, util2_sum = 0, drop_sum = 0;
        double nsys_sum = 0, qlen1_sum = 0, qlen2_sum = 0;

        for (int r = 0; r < RUNS; r++) {
            config.num_users = users;

            simulate(config);  // 🔥 main simulation happens here

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
        double ci = 3.291 * sd / sqrt(RUNS);

        outfile << users << "," << m << "," << m-ci << "," << m+ci << ","
                << throughput_sum/RUNS << "," << goodput_sum/RUNS << "," << badput_sum/RUNS << ","
                << util1_sum/RUNS << "," << util2_sum/RUNS << "," << drop_sum/RUNS << ","
                << nsys_sum/RUNS << "," << qlen1_sum/RUNS << "," << qlen2_sum/RUNS << "\n";
    }

    outfile.close();
}