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

# ==============================
# USER-DEFINED PARAMETERS (EDIT)
# ==============================

S1 = 0.1   # service time server 1
S2 = 0.05    # service time server 2
p  = 0.75    # probability of going to server 2
Z  = 5.0    # think time (set same as simulation mean)

# ==============================
# LOAD DATA
# ==============================

data = pd.read_csv(csv_file)
users = data["users"]

# ==============================
# MVA COMPUTATION
# ==============================

max_users = int(users.max())

R_mva = np.zeros(max_users + 1)
X_mva = np.zeros(max_users + 1)
N_mva = np.zeros(max_users + 1)

# per-node queue lengths
N1 = np.zeros(max_users + 1)
N2 = np.zeros(max_users + 1)

for n in range(1, max_users + 1):

    # residence times
    R1 = S1 * (1 + N1[n-1])
    R2 = S2 * (1 + N2[n-1])

    # total response time
    R = R1 + p * R2

    # throughput
    X = n / (Z + R)

    # update queue lengths
    N1[n] = X * R1
    N2[n] = X * p * R2

    R_mva[n] = R
    X_mva[n] = X
    N_mva[n] = N1[n] + N2[n]

# Map MVA values to user points
R_mva_plot = [R_mva[int(u)] for u in users]
X_mva_plot = [X_mva[int(u)] for u in users]
N_mva_plot = [N_mva[int(u)] for u in users]


U1_mva = np.zeros(max_users + 1)
U2_mva = np.zeros(max_users + 1)

for n in range(1, max_users + 1):
    U1_mva[n] = X_mva[n] * S1
    U2_mva[n] = X_mva[n] * p * S2

# Map to user points
U1_mva_plot = [U1_mva[int(u)] for u in users]
U2_mva_plot = [U2_mva[int(u)] for u in users]

# ==============================
# HELPER
# ==============================

def plot_single(y, ylabel, filename):
    plt.figure()
    plt.plot(users, y, marker='o')
    plt.xlabel("Number of Users")
    plt.ylabel(ylabel)
    plt.title(f"{ylabel} vs Number of Users")
    plt.grid()
    plt.savefig(os.path.join(output_dir, filename))
    plt.close()

# ==============================
# BASIC PLOTS
# ==============================

plot_single(data["drop_rate"], "Drop Rate", "drop_rate.png")

# ==============================
# RESPONSE TIME (SIM vs MVA)
# ==============================

plt.figure()
plt.plot(users, data["mean_rt"], marker='o', label="Simulation")
plt.plot(users, R_mva_plot, marker='x', linestyle='--', label="MVA")
plt.xlabel("Number of Users")
plt.ylabel("Response Time")
plt.title("Response Time: Simulation vs MVA")
plt.legend()
plt.grid()
plt.savefig(os.path.join(output_dir, "response_time_validation.png"))
plt.close()

# ==============================
# NUMBER IN SYSTEM (SIM vs MVA)
# ==============================

plt.figure()
plt.plot(users, data["avg_num_system"], marker='o', label="Simulation")
plt.plot(users, N_mva_plot, marker='x', linestyle='--', label="MVA")
plt.xlabel("Number of Users")
plt.ylabel("Avg Number in System")
plt.title("Number in System: Simulation vs MVA")
plt.legend()
plt.grid()
plt.savefig(os.path.join(output_dir, "num_system_validation.png"))
plt.close()

# ==============================
# THROUGHPUT VALIDATION (ONLY)
# ==============================

plt.figure()
plt.plot(users, data["throughput"], marker='o', label="Simulation")
plt.plot(users, X_mva_plot, marker='x', linestyle='--', label="MVA")
plt.xlabel("Number of Users")
plt.ylabel("Throughput")
plt.title("Throughput: Simulation vs MVA")
plt.legend()
plt.grid()
plt.savefig(os.path.join(output_dir, "throughput_validation.png"))
plt.close()

# ==============================
# THROUGHPUT / GOODPUT / BADPUT
# ==============================

plt.figure()
plt.plot(users, data["throughput"], marker='o', label="Throughput")
plt.plot(users, data["goodput"], marker='o', label="Goodput")
plt.plot(users, data["badput"], marker='o', label="Badput")
plt.xlabel("Number of Users")
plt.ylabel("Rate")
plt.title("Throughput vs Goodput vs Badput")
plt.legend()
plt.grid()
plt.savefig(os.path.join(output_dir, "throughput_goodput_badput.png"))
plt.close()

# ==============================
# UTILIZATION
# ==============================

plt.figure()

# Simulation
plt.plot(users, data["util_server1"], marker='o', label="Server 1 (Sim)")
plt.plot(users, data["util_server2"], marker='o', label="Server 2 (Sim)")

# MVA
plt.plot(users, U1_mva_plot, linestyle='--', marker='x', label="Server 1 (MVA)")
plt.plot(users, U2_mva_plot, linestyle='--', marker='x', label="Server 2 (MVA)")

plt.xlabel("Number of Users")
plt.ylabel("Utilization")
plt.title("Utilization: Simulation vs MVA")
plt.legend()
plt.grid()
plt.savefig(os.path.join(output_dir, "utilization_validation.png"))
plt.close()

# ==============================
# QUEUE LENGTHS
# ==============================

plt.figure()
plt.plot(users, data["avg_queue_length_s1"], marker='o', label="Queue 1")
plt.plot(users, data["avg_queue_length_s2"], marker='o', label="Queue 2")
plt.xlabel("Number of Users")
plt.ylabel("Average Queue Length")
plt.title("Queue Length vs Number of Users")
plt.legend()
plt.grid()
plt.savefig(os.path.join(output_dir, "queue_lengths.png"))
plt.close()

print(f"Plots saved in: {output_dir}")