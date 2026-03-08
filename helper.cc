#include <bits/stdc++.h>
using namespace std;

enum EventType {
    REQUEST_ARRIVAL = 1,
    REQUEST_DEPARTURE = 2,
    REQUEST_TIMEOUT = 3,
    CONTEXT_SWITCH = 4
};

struct Request {
    int req_id;
    int user_id;
    bool timed_out;

    // timestamps
    double issue_time;
    double thread_assigned_time;
    double completion_time;

    // service parameters
    double service_time;
    double timeout_time;

    Request(int r_id, int u_id, double issue) {
        req_id = r_id;
        user_id = u_id;
        issue_time = issue;
        thread_assigned_time = -1;
        completion_time = -1;
        service_time = 0;
        timeout_time = 0;
        timed_out = false;
    }
};

struct Event {
    double time;
    int event_type;
    int request_id;
    int core_id;

    Event(double t, int type, int req = -1, int core = -1) {
        time = t;
        event_type = type;
        request_id = req;
        core_id = core;
    }
    bool operator<(const Event& other) const {
        return time > other.time; 
    }
};

//global variables
priority_queue<Event> event_heap;
vector<Request> requests;          // store all requests for further statistics calculation
int global_request_counter = 0;
int MAX_REQUESTS = 1000;
int completed_requests = 0;
double current_time = 0;
default_random_engine generator;

double gaussian_sample(double mean, double stddev) {
    normal_distribution<double> dist(mean, stddev);
    return max(0.0, dist(generator)); 
}

class User {
public :
    int user_id;
    double think_time_mean;
    double think_time_std;
    double timeout;

    User(int uid, double think_mean, double think_std, double t) {
        user_id = uid;
        think_time_mean = think_mean;
        think_time_std = think_std;
        timeout = t;
    }

    void issue_req() {

        double think = gaussian_sample(think_time_mean, think_time_std);
        double issue_time = current_time + think;
        int req_id = global_request_counter++;
        Request req(req_id, user_id, issue_time);
        req.timeout_time = current_time + timeout;

        requests.push_back(req);
        event_heap.push(Event(issue_time, REQUEST_ARRIVAL, req_id));  // push arrival event;
        event_heap.push(Event(req.timeout_time, REQUEST_TIMEOUT, req_id));  // push timeout event;

    }

    void handle_reply(int req_id) {

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

    void handle_timeout(int req_id) {

        Request &req = requests[req_id];
        if (req.completion_time != -1) {
            // if the request is already finished do nothing. We can handle this in the driver also so this check is a little redundant;
            return;
        }
        issue_req();
        
    }
};