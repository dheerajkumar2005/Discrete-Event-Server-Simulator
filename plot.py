import pandas as pd
import matplotlib.pyplot as plt

data = pd.read_csv("response_time_vs_users_ci.csv")

users = data["users"]
mean = data["mean_rt"]
lower = data["lower_ci"]
upper = data["upper_ci"]

error = [mean - lower, upper - mean]

plt.errorbar(users, mean, yerr=error, fmt='o-', capsize=5)

plt.xlabel("Number of Users")
plt.ylabel("Average Response Time")
plt.title("Response Time vs Users (95% CI)")
plt.grid(True)
plt.savefig("./resp_time_vs_users.png")

plt.show()