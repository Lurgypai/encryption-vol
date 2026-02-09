import os

# generate <repeat> number of each specified dataset. each <repeat> value is a separate file
repeats = [4, 8, 16, 32]
sizes = ["1073741824"]
libs = ["none"]
algs = ["none"]

output_dir = "configs"

if len(sizes) != len(libs) or len(libs) != len(algs):
    print("invalid sizes")
    exit(1)

# Generate combinations and write config files
for repeat_count in repeats:
    filename = f"{repeat_count:03d}.config"
    filepath = os.path.join(output_dir, filename)
    with open(filepath, "w") as f:
        for i in range(len(sizes)):
            size = sizes[i]
            lib = libs[i]
            alg = algs[i]
            for j in range(repeat_count):
                f.write(f"dataset\n")
                f.write(f"count={size}\n")
                f.write(f"library={lib}\n")
                f.write(f"algorithm={alg}\n")
