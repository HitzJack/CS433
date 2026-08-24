import matplotlib.pyplot as plt
import re

# LaTeX table string (paste your data here)
latex_data = """
8 & 46644425\\
16 & 23569184\\
32 & 11896727\\
64 & 5998804\\
128 & 3089724\\
256 & 1776183\\
512 & 986628\\
1024 & 649643\\
2048 & 430397\\
4096 & 391283\\
8192 & 702962\\ 
"""

# Parsing the data using regular expressions
threads = []
series_1 = []
series_2 = []
omp = []
# Use regular expression to capture each row of the data
for line in latex_data.strip().splitlines():
    match = re.match(r'(\d+)\s*&\s*(\d+)\s*', line)
    if match:
        threads.append(int(match.group(1)))
        series_1.append(int(match.group(2)))
        # series_2.append(9428)
        omp.append(2070781)

print(series_1)
# Plotting
plt.figure(figsize=(10, 6))
plt.plot(threads, series_1, marker='o', label='CUDA Program')
# plt.plot(threads, series_2, label='Best Result after shared memory Optimization')
plt.plot(threads, omp, label='Best OpenMP Result')

# Set scale to logarithmic for better visibility
plt.xscale('log')
plt.yscale('log')

# Labels and title
plt.xlabel("Number of Threads")
plt.ylabel("Execution time (microseconds)")
plt.title("Execution time vs Number of Threads")
plt.legend()

# Show plot
plt.grid(True, which="both", linestyle="--", linewidth=0.5)
plt.show()
