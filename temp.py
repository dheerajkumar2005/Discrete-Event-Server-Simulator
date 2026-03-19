import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

if len(sys.argv) != 4:
    print("Usage: python plot_rt.py <csv_file1> <csv_file2> <output_directory>")
    sys.exit(1)

csv_file1 = sys.argv[1]
csv_file2 = sys.argv[2]
output_dir = sys.argv[3]

os.makedirs(output_dir, exist_ok=True)

# Read data
data1 = pd.read_csv(csv_file1)
data2 = pd.read_csv(csv_file2)

# Extract columns
users1 = data1["users"]
rt1 = data1["mean_rt"]

users2 = data2["users"]
rt2 = data2["mean_rt"]

# Labels (use filenames)
label1 = "FCFS"
label2 = "Round Robin"

# Plot
plt.figure()

plt.plot(users1, rt1, marker='o', linewidth=2, label=label1)
plt.plot(users2, rt2, marker='s', linewidth=2, label=label2)

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Average Response Time vs Number of Users")
plt.grid(True)
plt.legend()

# Save
output_path = os.path.join(output_dir, "avg_response_time_mixed_vs_users.png")
plt.savefig(output_path)
plt.close()

print(f"Plot saved to {output_path}")