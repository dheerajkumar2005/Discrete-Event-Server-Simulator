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

int main(int argc, char *argv[]) {

    if (argc >= 3) {
        return 1;
    }

    int NUM_USERS = 1;
    if (argc == 2) NUM_USERS = stoi(argv[1]);

    double THINK_TIME_MEAN = 5.0;
    double THINK_TIME_STD = 1.0;
    double TIMEOUT = 20.0;
    int NCORES = 4;
    int TOT_THREADS = 4;
    int QUEUE_CAP = 50;
    double SERVICE_TIME_MEAN = 0.5;
    double TIME_SLICE = 100;

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

    // cout << "Simulation Finished\n";
    // cout << "Completed Requests: " << completed_requests << endl;
    // cout << "Total Requests Generated: " << global_request_counter << endl;
    cout << "Average Response Time : " << compute_avg_response_time() << endl;


}