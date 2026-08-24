# List of compilation flags
BARRIERS = POSIX_BARRIER CENTRALIZED_BW_BARRIER TREE_BW_BARRIER CENTRALIZED_CV_BARRIER TREE_CV_BARRIER
OMP_BARRIERS = OMP_BARRIER
LOCKS = POSIX_LOCK BAKERY_LOCK SPIN_LOCK TTS_LOCK TICKET_LOCK ARRAY_LOCK SEMAPHORE_LOCK
OMP_LOCKS = OMP_LOCK

# Number of threads to test with
THREADS = 8#  2 4 8 # 2 4 8 16 # Modify based on your requirements

# Number of runs for each configuration
N = 1

# The output executable
EXECUTABLE = my_program

# The compiler and compiler flags
CC = gcc

# Default target
all: $(BARRIERS) $(OMP_BARRIERS) $(LOCKS) $(OMP_LOCKS)

# Function to run experiments and write results to CSV
# result=$$(./$(EXECUTABLE) $(t) | grep "Time Taken" | awk '{print "$1,$(t)," $$3}'); \
# echo $$result; \
# echo $$result >> "results/results_$1.csv"; \
			# @./$(EXECUTABLE) $(t)

define run_analysis
	@echo "Running experiments for $1"
	@echo "Flag,num_threads,Time"
	@echo "Flag,num_threads,Time" > "results/results_$1.csv"
			
	@$(foreach t, $(THREADS), \
		$(foreach i, $(shell seq 1 $(N)), \
			result=$$(./$(EXECUTABLE) $(t) | grep "Time Taken" | awk '{print "$1,$(t)," $$3}'); \
			echo $$result; \
			echo $$result >> "results/results_$1.csv"; \
		) \
	)
endef

$(BARRIERS):
	$(CC) -O3 -pthread -D$@ -o $(EXECUTABLE) pthread_main_barrier.c
	$(call run_analysis,$@)

$(OMP_BARRIERS):
	$(CC) -O3 -fopenmp -o $(EXECUTABLE) omp_main_barrier.c
	$(call run_analysis,$@)

$(LOCKS):
	$(CC) -O3 -pthread -D$@ -o $(EXECUTABLE) pthread_main_lock.c
	$(call run_analysis,$@)

$(OMP_LOCKS):
	$(CC) -O3 -fopenmp -o $(EXECUTABLE) omp_main_lock.c
	$(call run_analysis,$@)

BARRIERS: $(BARRIERS) $(OMP_BARRIERS)
LOCKS: $(LOCKS) $(OMP_LOCKS)

# Clean target to remove compiled files and the output CSV
clean:
	rm -f $(EXECUTABLE)
clean_outputs:
	rm -rf results
	mkdir results