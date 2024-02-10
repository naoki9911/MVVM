import pickle
import common_util
from multiprocessing import Pool

cmd = [
   "linpack","llama"
]
arg = [
    [],["stories15M.bin", "-z", "tokenizer.bin", "-t", "0.0"],
]


pool = Pool(processes=40)
results = []
def run_mvvm():
    global results
    results1 = []
    for i in range(len(cmd)):
        for j in range(len(common_util.aot_variant)):
            for env in common_util.list_of_arg:
                aot = cmd[i] + common_util.aot_variant[j]
                results1.append(pool.apply_async(common_util.run_checkpoint, (aot, arg[i], env)))
    # print the results
    results += [x.get() for x in results1]


def run_criu():
    global results
    results1 = []

    for i in range(len(cmd)):
        aot = cmd[i]
        results1.append(
            pool.apply_async(
                common_util.run_criu, (aot, "linpack", arg[i], "OMP_NUM_THREADS=1")
            )
        )
    # print the results
    results += [x.get() for x in results1]


def run_qemu_x86_64():
    global results
    results1 = []

    for i in range(len(cmd)):
        aot = cmd[i]
        results1.append(
            pool.apply_async(
                common_util.run_qemu_x86_64, (aot, "linpack", arg[i], "OMP_NUM_THREADS=1")
            )
        )
    # print the results
    results += [x.get() for x in results1]


run_criu()
run_qemu_x86_64()

# print the results

run_qemu_x86_64()
run_criu()
with open("bench_migration_results.pickle", "wb") as f:
    pickle.dump(results, f)
for exec, output in results:
    print(exec)
    lines = output.split("\n")
    for line in lines:
        if (
            line.__contains__("Execution time:")
            or line.__contains__("real")
            or line.__contains__("user")
        ):
            print(line)

# read the results
with open("bench_migration_results.pickle", "rb") as f:
    results = pickle.load(f)