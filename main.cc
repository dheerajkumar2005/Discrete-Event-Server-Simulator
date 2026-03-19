#include "simulator.hh"
#include <fstream>



int main() {

    Config config;

    config.load_config("config.txt");
    int RUNS = config.runs;
    ofstream outfile("metrics.csv");
    outfile << "users,mean_rt,lower_ci,upper_ci,throughput,goodput,badput,utilization,drop_rate,avg_num_system,avg_queue_length\n";

    for (int users = config.min_users; users <= config.max_users; users += config.user_step_size) {
        vector<double> samples;
        double throughput_sum = 0;
        double goodput_sum = 0;
        double badput_sum = 0;
        double util_sum = 0;
        double drop_sum = 0;
        double nsys_sum = 0;
        double qlen_sum = 0;

        for (int r = 0; r < config.runs; r++) {
            simulate(config,users);
            Metrics m;
            m = compute_metrics(config);
            samples.push_back(m.avg_response);
            throughput_sum += m.throughput;
            goodput_sum += m.goodput;
            badput_sum += m.badput;
            util_sum += m.utilization;
            drop_sum += m.avg_drop_rate;
            nsys_sum += m.avg_num_system;
            qlen_sum += m.avg_queue_length;

            reset();
        }

        double m = mean(samples);
        double sd = stddev(samples, m);
        double ci = 2.576* sd / sqrt(RUNS);  // 99% confidence interval
        double lower = m - ci;
        double upper = m + ci;

        double throughput = throughput_sum / RUNS;
        double goodput = goodput_sum / RUNS;
        double badput = badput_sum / RUNS;
        double util = util_sum / RUNS;
        double drop_rate = drop_sum / RUNS;
        double avg_nsys = nsys_sum / RUNS;
        double avg_qlen = qlen_sum / RUNS;

        outfile << users << "," << m << "," << lower << "," << upper << "," << throughput << "," << goodput << "," << badput << "," << util << "," << drop_rate << "," << avg_nsys << "," << avg_qlen << "\n";
    }

    outfile.close();
}

    // cout << "Simulation Finished\n";
    // cout << "Completed Requests: " << completed_requests << endl;
    // cout << "Total Requests Generated: " << global_request_counter << endl;
    // cout << "Average Response Time : " << compute_avg_response_time() << endl;

