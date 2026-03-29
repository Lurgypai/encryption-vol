import os

# region_counts = [ 512 ]
# total dataset sizes
# sizes = [(32 * 1024 * 1024)]

region_counts = [1, 8, 16, 32]
sizes = [32]

libs = ["gcrypt"]
algs = ["aes256"]

output_dir = "configs"

if len(libs) != len(algs):
    print("invalid sizes")
    exit(1)

# Generate combinations and write config files
for region_count in region_counts:
    for size in sizes:
        filename = f"{region_count:04d}r-{int(size / 1024):08d}MiB.config"
        filepath = os.path.join(output_dir, filename)
        with open(filepath, "w") as f:
            f.write(f"dataset\n")
            for i in range(region_count):
                index = i % len(libs)
                lib = libs[index]
                alg = algs[index]
                f.write("region\n")
                f.write(f"size={int(size/region_count)}\n")
                f.write(f"library={lib}\n")
                f.write(f"algorithm={alg}\n")
