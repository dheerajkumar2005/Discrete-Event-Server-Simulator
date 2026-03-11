import pandas as pd
import matplotlib.pyplot as plt
import os
import sys

if len(sys.argv) != 3:
    print("Usage: python plot.py <csv_file> <output_directory>")
    sys.exit(1)

csv_file = sys.argv[1]
output_dir = sys.argv[2]

os.makedirs(output_dir, exist_ok=True)

data = pd.read_csv(csv_file)

users = data["users"]

# -------------------------------------------------
# 1. Average Response Time vs Users (with CI)
# -------------------------------------------------

mean_rt = data["mean_rt"]
lower_ci = data["lower_ci"]
upper_ci = data["upper_ci"]

error = [mean_rt - lower_ci, upper_ci - mean_rt]

plt.figure()

plt.errorbar(
    users,
    mean_rt,
    yerr=error,
    fmt='o-',
    capsize=5,
    label="Average Response Time"
)

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Average Response Time vs Number of Users")
plt.grid(True)
plt.legend()

plt.savefig(os.path.join(output_dir, "avg_response_time_vs_users.png"))
plt.close()


# -------------------------------------------------
# 2. Throughput / Goodput / Badput
# -------------------------------------------------

throughput = data["throughput"]
goodput = data["goodput"]
badput = data["badput"]

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


# -------------------------------------------------
# 3. Average Core Utilization
# -------------------------------------------------

util = data["utilization"]

plt.figure()

plt.plot(users, util, marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Average Core Utilization")
plt.title("Core Utilization vs Number of Users")
plt.grid(True)

plt.savefig(os.path.join(output_dir, "utilization_vs_users.png"))
plt.close()


# -------------------------------------------------
# 4. Drop Rate
# -------------------------------------------------

drop_rate = data["drop_rate"]

plt.figure()

plt.plot(users, drop_rate, marker='o')

plt.xlabel("Number of Users")
plt.ylabel("Drop Rate")
plt.title("Drop Rate vs Number of Users")
plt.grid(True)

plt.savefig(os.path.join(output_dir, "drop_rate_vs_users.png"))
plt.close()


print("Plots saved to:", output_dir)