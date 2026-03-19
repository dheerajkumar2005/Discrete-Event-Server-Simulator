import os
import subprocess
import sys

# --- Configuration ---
# Assuming 'sim' and 'new_plot.py' are in the current directory alongside the group folders
BASE_DIR = os.path.abspath("./experiments/experiment-1/") 
SIM_EXEC = os.path.abspath("./sim") # Change to "sim.exe" if on Windows
PLOT_SCRIPT = os.path.abspath("./new_plot.py")
OUTPUT_DIR = os.path.abspath("./experiments/experiment-1/")

def main():
    if not os.path.exists(SIM_EXEC):
        print(f"Error: Executable not found at {SIM_EXEC}")
        sys.exit(1)

    generated_csvs = []

    # 1. Loop through all items in the base directory
    for item in os.listdir(BASE_DIR):
        folder_path = os.path.join(BASE_DIR, item)
        
        # Check if it's a directory and contains a config.txt
        if os.path.isdir(folder_path):
            config_path = os.path.join(folder_path, "config.txt")
            
            if os.path.exists(config_path):
                print(f"\n--- Running sim in: {item} ---")
                
                # Change working directory to the subfolder so 'sim' finds config.txt locally
                os.chdir(folder_path)
                
                try:
                    # Execute the simulation
                    subprocess.run([SIM_EXEC], check=True)
                    
                    # Track the successfully generated metrics.csv
                    csv_path = os.path.join(folder_path, "metrics.csv")
                    if os.path.exists(csv_path):
                        generated_csvs.append(csv_path)
                    else:
                        print(f"Warning: metrics.csv not found in {item} after running sim.")
                        
                except subprocess.CalledProcessError as e:
                    print(f"Error running sim in {item}: {e}")
                
                # Always return to the base directory before the next iteration
                os.chdir(BASE_DIR)

    # 2. Run the plotting script if we have CSVs
    if generated_csvs:
        print("\n--- Generating Combined Plots ---")
        plot_command = [sys.executable, PLOT_SCRIPT, OUTPUT_DIR] + generated_csvs
        
        try:
            subprocess.run(plot_command, check=True)
            print(f"\nDone! All plots are saved in: {OUTPUT_DIR}")
        except subprocess.CalledProcessError as e:
            print(f"Error generating plots: {e}")
    else:
        print("\nNo metrics.csv files were generated. Skipping plotting.")

if __name__ == "__main__":
    main()