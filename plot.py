import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import sys
import os

# -----------------------------
# CONFIG (match your simulator)
# -----------------------------
Z = 5.0
S1 = 1.0
S2 = 1.0
p = 0.75
m1 = 4   # server1 cores
m2 = 4   # server2 cores

# -----------------------------
# MVA for 2-node tandem system
# -----------------------------
def mva_tandem(N_max):
    R1 = np.zeros(N_max + 1)
    R2 = np.zeros(N_max + 1)
    Q1 = np.zeros(N_max + 1)
    Q2 = np.zeros(N_max + 1)
    X = np.zeros(N_max + 1)

    m1 = 4
    m2 = 4

    for N in range(1, N_max + 1):

        # Multi-server residence time
        R1[N] = S1 * (1 + Q1[N-1] / m1)
        R2[N] = S2 * (1 + Q2[N-1] / m2)

        R_total = R1[N] + p * R2[N]

        X[N] = N / (Z + R_total)

        Q1[N] = X[N] * R1[N]
        Q2[N] = X[N] * p * R2[N]

    return X, R1, R2, Q1, Q2

# -----------------------------
# MAIN
# -----------------------------
if len(sys.argv) != 3:
    print("Usage: python plot.py metrics.csv output_dir")
    sys.exit(1)

csv_file = sys.argv[1]
out_dir = sys.argv[2]
os.makedirs(out_dir, exist_ok=True)

df = pd.read_csv(csv_file)

users = df["users"].values
N_max = int(max(users))

# Run MVA
X_mva, R1_mva, R2_mva, Q1_mva, Q2_mva = mva_tandem(N_max)

R_total_mva = R1_mva + p * R2_mva
N_system_mva = X_mva * R_total_mva   # Little's Law

# Align MVA with user points
R_mva_plot = [R_total_mva[n] for n in users]
X_mva_plot = [X_mva[n] for n in users]
Q1_mva_plot = [Q1_mva[n] for n in users]
Q2_mva_plot = [Q2_mva[n] for n in users]
Nsys_mva_plot = [N_system_mva[n] for n in users]

# -----------------------------
# 1. Response Time
# -----------------------------
plt.figure()

# Mean line
plt.plot(users, df["mean_rt"], label="Simulation", marker='o')

# Confidence interval band
plt.fill_between(
    users,
    df["lower_ci"],
    df["upper_ci"],
    alpha=0.2,
    label="Confidence Interval"
)

# MVA
plt.plot(users, R_mva_plot, linewidth=2, label="MVA")

plt.xlabel("Number of Users")
plt.ylabel("Response Time")
plt.title("Response Time vs Users")
plt.legend()
plt.grid(True)

plt.savefig(f"{out_dir}/response_time.png")
plt.close()

# -----------------------------
# 2. Throughput / Goodput / Badput
# -----------------------------
plt.figure()
plt.plot(users, df["throughput"], marker='o', label="Throughput (Sim)")
plt.plot(users, df["goodput"], marker='s', label="Goodput (Sim)")
plt.plot(users, df["badput"], marker='^', label="Badput (Sim)")
plt.plot(users, X_mva_plot, linestyle='--', label="Throughput (MVA)")

plt.xlabel("Number of Users")
plt.ylabel("Rate")
plt.title("Throughput / Goodput / Badput vs Users")
plt.legend()
plt.grid(True)
plt.savefig(f"{out_dir}/throughput_goodput_badput.png")
plt.close()

# -----------------------------
# 3. Utilization
# -----------------------------
plt.figure()
plt.plot(users, df["util_server1"], label="Server1 Util (Sim)")
plt.plot(users, df["util_server2"], label="Server2 Util (Sim)")

util1_mva = np.array(X_mva_plot) * S1 / m1
util2_mva = np.array(X_mva_plot) * p * S2 / m2

plt.plot(users, util1_mva, linestyle='--', label="Server1 Util (MVA)")
plt.plot(users, util2_mva, linestyle='--', label="Server2 Util (MVA)")

plt.xlabel("Number of Users")
plt.ylabel("Utilization")
plt.title("Utilization vs Users")
plt.legend()
plt.grid(True)
plt.savefig(f"{out_dir}/utilization.png")
plt.close()

# -----------------------------
# 4. Queue Lengths
# -----------------------------
plt.figure()
plt.plot(users, df["avg_queue_length_s1"], label="Queue1 (Sim)")
plt.plot(users, df["avg_queue_length_s2"], label="Queue2 (Sim)")

plt.plot(users, Q1_mva_plot, linestyle='--', label="Queue1 (MVA)")
plt.plot(users, Q2_mva_plot, linestyle='--', label="Queue2 (MVA)")

plt.xlabel("Number of Users")
plt.ylabel("Queue Length")
plt.title("Queue Length vs Users")
plt.legend()
plt.grid(True)
plt.savefig(f"{out_dir}/queue_lengths.png")
plt.close()

# -----------------------------
# 5. Number in System
# -----------------------------
plt.figure()
plt.plot(users, df["avg_num_system"], marker='o', label="Simulated")

plt.plot(users, Nsys_mva_plot, linestyle='--', label="MVA")

plt.xlabel("Number of Users")
plt.ylabel("Average Number in System")
plt.title("Number in System vs Users")
plt.legend()
plt.grid(True)
plt.savefig(f"{out_dir}/num_in_system.png")
plt.close()

# -----------------------------
# 6. Drop Rate
# -----------------------------
plt.figure()
plt.plot(users, df["drop_rate"], marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Drop Rate")
plt.title("Drop Rate vs Users")
plt.grid(True)
plt.savefig(f"{out_dir}/drop_rate.png")
plt.close()

print("Plots saved in:", out_dir)