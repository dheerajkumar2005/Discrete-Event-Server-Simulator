import pandas as pd
import matplotlib.pyplot as plt
import os
import sys
import numpy as np

# Usage: python plot.py <output_directory> <csv_file1> [label1] <csv_file2> [label2] ...
#
# Labels are optional. If not provided, the csv filename is used as the label.
#
# Examples:
#   python plot.py out/ metrics1.csv metrics2.csv
#   python plot.py out/ metrics1.csv "Config A" metrics2.csv "Config B"

def parse_args(argv):
    """
    Parse: output_dir followed by alternating csv / optional-label pairs.
    Heuristic: if an arg doesn't end in .csv, treat it as the label for the preceding csv.
    """
    if len(argv) < 3:
        print("Usage: python plot.py <output_directory> <csv1> [label1] <csv2> [label2] ...")
        sys.exit(1)

    output_dir = argv[1]
    entries = []   # list of (csv_path, label)

    i = 2
    while i < len(argv):
        csv_path = argv[i]
        if not csv_path.endswith(".csv"):
            print(f"ERROR: Expected a .csv file, got: {csv_path}")
            sys.exit(1)
        # Check if next arg is a label (doesn't end in .csv)
        if i + 1 < len(argv) and not argv[i + 1].endswith(".csv"):
            label = argv[i + 1]
            i += 2
        else:
            label = os.path.splitext(os.path.basename(csv_path))[0]
            i += 1
        entries.append((csv_path, label))

    return output_dir, entries


output_dir, entries = parse_args(sys.argv)
os.makedirs(output_dir, exist_ok=True)

# Load all datasets
datasets = []
for csv_path, label in entries:
    data = pd.read_csv(csv_path)
    datasets.append((label, data))

# Color cycle so each dataset gets a distinct color across all plots
colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
def color(i):
    return colors[i % len(colors)]


# -----------------------------
# Mean Value Analysis (unchanged, kept for reference)
# -----------------------------

Z = 100.0
S = 10.0
m = 8


# -----------------------------
# Response Time
# -----------------------------

plt.figure()

for i, (label, data) in enumerate(datasets):
    users    = data["users"]
    mean_rt  = data["mean_rt"]
    error    = [mean_rt - data["lower_ci"], data["upper_ci"] - mean_rt]

    plt.errorbar(
        users, mean_rt, yerr=error,
        fmt='o-',
        markersize=1,
        linewidth=1,
        capsize=2,
        color=color(i),
        label=f"{label} (mean ± CI)"
    )

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Response Time vs Users")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "response_time_vs_users.png"))
plt.close()


# -----------------------------
# Number in System
# -----------------------------

plt.figure()

for i, (label, data) in enumerate(datasets):
    users          = data["users"]
    throughput     = data["throughput"]
    mean_rt        = data["mean_rt"]
    avg_num_system = data["avg_num_system"]
    N_little       = throughput * mean_rt

    plt.plot(users, avg_num_system,
             marker='o', markersize=4, linewidth=2.5,
             color=color(i), label=f"{label} - Simulation")

    plt.plot(users, N_little,
             linestyle='-.', marker='x', markersize=4, linewidth=2,
             color=color(i), label=f"{label} - Little's Law (X·R)")

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

for i, (label, data) in enumerate(datasets):
    users     = data["users"]
    throughput = data["throughput"]
    goodput   = data["goodput"]
    badput    = data["badput"]
    c = color(i)

    plt.plot(users, throughput, marker='o',  color=c, linestyle='-',  label=f"{label} - Throughput")
    plt.plot(users, goodput,   marker='s',  color=c, linestyle='--', label=f"{label} - Goodput")
    plt.plot(users, badput,    marker='^',  color=c, linestyle=':',  label=f"{label} - Badput")

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

for i, (label, data) in enumerate(datasets):
    plt.plot(data["users"], data["utilization"],
             marker='o', color=color(i), label=label)

plt.xlabel("Number of Users")
plt.ylabel("Core Utilization")
plt.title("Utilization vs Users")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "utilization_vs_users.png"))
plt.close()


# -----------------------------
# Drop Rate
# -----------------------------

plt.figure()

for i, (label, data) in enumerate(datasets):
    plt.plot(data["users"], data["drop_rate"],
             marker='o', color=color(i), label=label)

plt.xlabel("Number of Users")
plt.ylabel("Drop Rate")
plt.title("Drop Rate vs Users")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "drop_rate_vs_users.png"))
plt.close()


# -----------------------------
# Average Queue Length
# -----------------------------

plt.figure()

for i, (label, data) in enumerate(datasets):
    plt.plot(data["users"], data["avg_queue_length"],
             marker='o', color=color(i), label=label)

plt.xlabel("Number of Users")
plt.ylabel("Average Queue Length")
plt.title("Average Queue Length vs Users")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "avg_queue_length_vs_users.png"))
plt.close()

print(f"Saved {len(datasets)} series × 6 plots to: {output_dir}")