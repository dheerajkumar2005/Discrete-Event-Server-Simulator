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

avg_num_system = data["avg_num_system"]

error = [mean_rt - lower_ci, upper_ci - mean_rt]


# -----------------------------
# Mean Value Analysis
# -----------------------------

Z = 5.0          # think time
S = 0.5          # mean service time
m = 8          # number of servers

max_users = int(users.max())

R_mva = np.zeros(max_users + 1)
X_mva = np.zeros(max_users + 1)
N_mva = np.zeros(max_users + 1)

for n in range(1, max_users + 1):

    Q = N_mva[n-1]

    # Correct multi-server approximation
    if Q < m:
        R = S
    else:
        R = S * (1 + (Q - m) / m)

    X = n / (Z + R)
    N = X * R

    R_mva[n] = R
    X_mva[n] = X
    N_mva[n] = N


R_mva_plot = [R_mva[int(n)] for n in users]
X_mva_plot = [X_mva[int(n)] for n in users]
Q_mva_plot = [N_mva[int(n)] for n in users]


# -----------------------------
# Response Time plot
# -----------------------------

plt.figure()

# Simulation with smaller markers + thinner line
plt.errorbar(
    users,
    mean_rt,
    yerr=error,
    fmt='o-',           # line + marker
    markersize=1,       # 🔥 reduced bead size
    linewidth=1,
    capsize=2,
    label="Simulation (mean ± CI)"
)

# MVA curve (make it visually distinct)
plt.plot(
    users,
    R_mva_plot,
    linestyle='--',
    linewidth=2,
    label="MVA Prediction"
)

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Response Time vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "response_time_vs_users.png"))
plt.close()

# -----------------------------
# Throughput plot
# -----------------------------

plt.figure()

plt.plot(users, throughput, marker='o', label="Simulation")
plt.plot(users, X_mva_plot, marker='s', label="MVA")

plt.xlabel("Number of Users")
plt.ylabel("Throughput")
plt.title("Throughput vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "throughput_vs_users.png"))
plt.close()


# -----------------------------
# Number in System
# -----------------------------

plt.figure()

plt.plot(users, avg_num_system, marker='o', label="Simulation")
plt.plot(users, Q_mva_plot, marker='s', label="MVA")

plt.xlabel("Number of Users")
plt.ylabel("Average Number in System")
plt.title("Number in System vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "number_in_system_vs_users.png"))
plt.close()


# -----------------------------
# Throughput / Goodput / Badput
# -----------------------------

plt.figure()

plt.plot(users, throughput, marker='o', label="Throughput")
plt.plot(users, goodput, marker='s', label="Goodput")
plt.plot(users, badput, marker='^', label="Badput")

plt.xlabel("Number of Users")
plt.ylabel("Rate")
plt.title("Throughput / Goodput / Badput vs Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "throughput_goodput_badput_vs_users.png"))
plt.close()


# -----------------------------
# Utilization
# -----------------------------

plt.figure()

plt.plot(users, util, marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Core Utilization")
plt.title("Utilization vs Users")
plt.grid(True)

plt.savefig(os.path.join(output_dir, "utilization_vs_users.png"))
plt.close()


# -----------------------------
# Drop Rate
# -----------------------------

plt.figure()

plt.plot(users, drop_rate, marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Drop Rate")
plt.title("Drop Rate vs Users")
plt.grid(True)

plt.savefig(os.path.join(output_dir, "drop_rate_vs_users.png"))
plt.close()