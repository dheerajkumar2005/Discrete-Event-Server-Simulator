import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import numpy as np

if len(sys.argv) != 3:
    print("Usage: python plot.py <csv_file> <output_directory>")
    sys.exit(1)

csv_file = sys.argv[1]
output_dir = sys.argv[2]

os.makedirs(output_dir, exist_ok=True)

data = pd.read_csv(csv_file)

users = data["users"]

mean_rt = data["mean_rt"]
lower_ci = data["lower_ci"]
upper_ci = data["upper_ci"]

throughput = data["throughput"]
goodput = data["goodput"]
badput = data["badput"]

util = data["utilization"]
drop_rate = data["drop_rate"]

if "avg_num_system" in data.columns:
    avg_num_system = data["avg_num_system"]
else:
    avg_num_system = throughput * mean_rt

if "avg_queue_length" in data.columns:
    avg_queue = data["avg_queue_length"]
else:
    avg_queue = None


error = [mean_rt - lower_ci, upper_ci - mean_rt]


# ---------------------------
# Mean Value Analysis
# ---------------------------

Z = 5.0
S = 0.5
m = 4
S_eff = S / m

max_users = int(users.max())

R_mva = np.zeros(max_users + 1)
X_mva = np.zeros(max_users + 1)
N_mva = np.zeros(max_users + 1)

Q_prev = 0

for n in range(1, max_users + 1):

    R = S_eff * (1 + Q_prev)

    X = n / (R + Z)

    Q = X * R

    R_mva[n] = R
    X_mva[n] = X
    N_mva[n] = Q

    Q_prev = Q


R_mva_plot = [R_mva[int(n)] for n in users]
X_mva_plot = [X_mva[int(n)] for n in users]
N_mva_plot = [N_mva[int(n)] for n in users]


# ---------------------------
# Response Time plot
# ---------------------------

plt.figure()

plt.errorbar(
    users,
    mean_rt,
    yerr=error,
    fmt='o-',
    capsize=5,
    label="Simulated"
)

plt.plot(users, R_mva_plot, marker='s', label="MVA Prediction")

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Response Time vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "response_time_vs_users.png"))
plt.close()


# ---------------------------
# Throughput plot
# ---------------------------

plt.figure()

plt.plot(users, throughput, marker='o', label="Simulated")
plt.plot(users, X_mva_plot, marker='s', label="MVA Prediction")

plt.xlabel("Number of Users")
plt.ylabel("Throughput")
plt.title("Throughput vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "throughput_vs_users.png"))
plt.close()


# ---------------------------
# Avg number in system
# ---------------------------

plt.figure()

plt.plot(users, avg_num_system, marker='o', label="Simulated")
plt.plot(users, N_mva_plot, marker='s', label="MVA Prediction")

plt.xlabel("Number of Users")
plt.ylabel("Average Number in System")
plt.title("Number in System vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "number_in_system_vs_users.png"))
plt.close()


# ---------------------------
# Throughput / Goodput / Badput
# ---------------------------

plt.figure()

plt.plot(users, throughput, marker='o', label="Throughput")
plt.plot(users, goodput, marker='s', label="Goodput")
plt.plot(users, badput, marker='^', label="Badput")

plt.xlabel("Number of Users")
plt.ylabel("Rate (requests / unit time)")
plt.title("Throughput / Goodput / Badput vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "throughput_goodput_badput_vs_users.png"))
plt.close()


# ---------------------------
# Utilization
# ---------------------------

plt.figure()

plt.plot(users, util, marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Average Core Utilization")
plt.title("Core Utilization vs Users")
plt.grid(True)

plt.savefig(os.path.join(output_dir, "utilization_vs_users.png"))
plt.close()


# ---------------------------
# Drop Rate
# ---------------------------

plt.figure()

plt.plot(users, drop_rate, marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Drop Rate")
plt.title("Drop Rate vs Users")
plt.grid(True)

plt.savefig(os.path.join(output_dir, "drop_rate_vs_users.png"))
plt.close()


# ---------------------------
# Queue Length
# ---------------------------

if avg_queue is not None:

    plt.figure()

    plt.plot(users, avg_queue, marker='o')

    plt.xlabel("Number of Users")
    plt.ylabel("Average Queue Length")
    plt.title("Queue Length vs Users")
    plt.grid(True)

    plt.savefig(os.path.join(output_dir, "queue_length_vs_users.png"))
    plt.close()