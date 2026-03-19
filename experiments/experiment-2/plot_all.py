import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

if len(sys.argv) < 3:
    print("Usage: python plot_all.py <output_directory> <path_to_csv_1> <path_to_csv_2> ...")
    sys.exit(1)

output_dir = sys.argv[1]
csv_files = sys.argv[2:]

os.makedirs(output_dir, exist_ok=True)

# Load all datasets and assign a label based on their parent folder name
datasets = []
for csv_file in csv_files:
    df = pd.read_csv(csv_file)
    
    # Extract the parent directory name to use as the legend label (e.g., "group1", "group2")
    label = os.path.basename(os.path.dirname(os.path.abspath(csv_file)))
    if not label:
        label = os.path.basename(csv_file)
        
    datasets.append({"label": label, "df": df})

# -----------------------------
# Response Time plot
# -----------------------------
plt.figure()
for data in datasets:
    df = data["df"]
    label = data["label"]
    error = [df["mean_rt"] - df["lower_ci"], df["upper_ci"] - df["mean_rt"]]
    
    plt.errorbar(
        df["users"], df["mean_rt"], yerr=error, fmt='o-', 
        markersize=3, linewidth=1.5, capsize=3, label=f"{label}"
    )

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Response Time vs Users (Comparison)")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "compare_response_time.png"))
plt.close()

# -----------------------------
# Number in System
# -----------------------------
plt.figure()
for data in datasets:
    df = data["df"]
    label = data["label"]
    
    # Plot Simulation Data
    line = plt.plot(df["users"], df["avg_num_system"], marker='o', markersize=4, linewidth=2, label=f"{label} (Sim)")[0]
    
    # Plot Little's Law using the same color but different style
    N_little = df["throughput"] * df["mean_rt"]
    plt.plot(df["users"], N_little, linestyle='-.', marker='x', markersize=4, linewidth=1.5, color=line.get_color(), label=f"{label} (Little's)")

plt.xlabel("Number of Users")
plt.ylabel("Average Number in System")
plt.title("Number in System vs Users (Comparison)")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "compare_number_in_system.png"))
plt.close()

# -----------------------------
# Throughput / Goodput / Badput
# -----------------------------
plt.figure()
for data in datasets:
    df = data["df"]
    label = data["label"]
    
    # Base plot for Throughput to grab the assigned color
    line = plt.plot(df["users"], df["throughput"], marker='o', linewidth=2, label=f"{label} (Throughput)")[0]
    color = line.get_color()
    
    # Goodput and Badput use the same color, different marker/linestyle
    plt.plot(df["users"], df["goodput"], marker='s', linestyle='--', color=color, label=f"{label} (Goodput)")
    plt.plot(df["users"], df["badput"], marker='^', linestyle=':', color=color, label=f"{label} (Badput)")

plt.xlabel("Number of Users")
plt.ylabel("Rate")
plt.title("Throughput / Goodput / Badput vs Users (Comparison)")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "compare_throughput_rates.png"))
plt.close()

# -----------------------------
# Utilization
# -----------------------------
plt.figure()
for data in datasets:
    df = data["df"]
    plt.plot(df["users"], df["utilization"], marker='o', label=data["label"])

plt.xlabel("Number of Users")
plt.ylabel("Core Utilization")
plt.title("Utilization vs Users (Comparison)")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "compare_utilization.png"))
plt.close()

# -----------------------------
# Drop Rate
# -----------------------------
plt.figure()
for data in datasets:
    df = data["df"]
    plt.plot(df["users"], df["drop_rate"], marker='o', label=data["label"])

plt.xlabel("Number of Users")
plt.ylabel("Drop Rate")
plt.title("Drop Rate vs Users (Comparison)")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "compare_drop_rate.png"))
plt.close()

# -----------------------------
# Average Queue Length
# -----------------------------
plt.figure()
for data in datasets:
    df = data["df"]
    if "avg_queue_length" in df.columns:
        plt.plot(df["users"], df["avg_queue_length"], marker='o', label=data["label"])

plt.xlabel("Number of Users")
plt.ylabel("Average Queue Length")
plt.title("Average Queue Length vs Users (Comparison)")
plt.grid(True)
plt.legend()
plt.savefig(os.path.join(output_dir, "compare_avg_queue_length.png"))
plt.close()

print(f"Successfully generated comparison plots in '{output_dir}/'")