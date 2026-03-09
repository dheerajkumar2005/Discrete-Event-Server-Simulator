#include "simulator.hh"

int main() {

    int NUM_USERS = 10;
    double THINK_TIME_MEAN = 5.0;
    double THINK_TIME_STD = 1.0;
    double TIMEOUT = 20.0;
    int NCORES = 4;
    int TOT_THREADS = 8;
    int QUEUE_CAP = 50;
    double SERVICE_TIME_MEAN = 3.0;
    double TIME_SLICE = 0.5;

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
                if (req.completion_time == -1 && !req.dropped) {
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

    cout << "Simulation Finished\n";
    cout << "Completed Requests: " << completed_requests << endl;
    cout << "Total Requests Generated: " << global_request_counter << endl;

    return 0;
}