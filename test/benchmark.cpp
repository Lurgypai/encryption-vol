#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

#include <cstring>

#include <hdf5.h>
#include <mpi.h>

extern "C" {
#include "enc_wrapper.h"
#include "../vol-encrypt/encrypt_vol_connector.h"
};

class Timer {
public:
    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }
    double getElapsed() {
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now() - start);
        return elapsed.count();
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

// size of elements in the opaque type, the count in the configuration is in terms of these blocks
constexpr std::size_t SHARED_BLOCK_SIZE = 16;

struct Region {
    std::int64_t size;
    std::string algorithm;
    std::string library;
};

struct Dataset {
    size_t size;
    std::vector<Region> regions;
};


static inline bool isValidRegion(const Region& dataset) {
    return dataset.size > 0 &&
        (dataset.algorithm == "aes256" || dataset.algorithm == "chacha20" || dataset.algorithm == "none") &&
        (dataset.library == "nettle" || dataset.library == "gcrypt" || dataset.library == "dummy" || dataset.library == "none");
}

static inline bool isValidDataset(const Dataset& dataset) {
    return dataset.size > 0 && dataset.regions.size() > 0;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int my_rank, rank_count;
    MPI_Comm_size(MPI_COMM_WORLD, &rank_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    if(argc != 3) {
        if(my_rank == 0) {
            std::cout << "Incorrect usage.\n";
            std::cout << "Usage: " << argv[0] << " <config> <write/read>\n";
        }
        return 1;
    }

    bool doWrite = false;
    if(std::string{argv[2]} == "write") {
        doWrite = true;
    } else if (std::string{argv[2]} == "read") {
        doWrite = false;
    }
    else {
        if(my_rank == 0) {
            std::cout << "Incorrect usage.\n";
            std::cout << "Usage: " << argv[0] << " <config> <write/read>\n";
        }
        return 1;
    }

    std::string configFileName{argv[1]};
    std::ifstream configFile{configFileName};

    if(!configFile.good()) {
        if(my_rank == 0) std::cerr << "ERROR: Unable to open file \"" << configFileName << "\"\n";
        return 1;
    }

    /* =========================== PARSE CONFIG ========================== */
    if(my_rank == 0) std::cout << "Parsing config \"" << configFileName << "\"" << std::endl;
    std::vector<Dataset> datasetTemplates;
    Dataset* curDataset = nullptr;
    Region* curRegion = nullptr;
    std::string line;
    while(std::getline(configFile, line)) {
        // check for beginning of dataset
        if(line == "dataset") {
            // generate new dataset
            if(curDataset != nullptr && !isValidDataset(*curDataset)) {
                if(my_rank == 0) std::cerr << "ERROR: Parsed emtpy dataset\n";
                return 1;
            }
            if(curRegion != nullptr && !isValidRegion(*curRegion)) {
                if(my_rank == 0) std::cerr << "ERROR: Parsed Invalid region\n";
                return 1;
            }
            datasetTemplates.emplace_back(Dataset{});
            curDataset = &datasetTemplates.back();
        }
        else if (line == "region") {
            if(curRegion != nullptr && !isValidRegion(*curRegion)) {
                if(my_rank == 0) std::cerr << "ERROR: Invalid region\n";
                return 1;
            }
            curDataset->regions.emplace_back(Region{});
            curRegion = &curDataset->regions.back();
        }

        // fill in dataset values
        else {
            if(curDataset == nullptr || curRegion == nullptr) {
                if(my_rank == 0) std::cerr << "ERROR: Initial dataset or region hasn't been started (did you forget \"dataset\" at the beginning of the file?\n";
                return 1;
            }
            // split line at = sign
            auto splitPos = line.find('=');
            if(splitPos == std::string::npos || splitPos == line.size() - 1) {
                if(my_rank == 0) std::cerr << "ERROR: Unable to parse line \"" << line << "\"\n";
                return 1;
            }

            std::string front = line.substr(0, splitPos);
            std::string back = line.substr(splitPos + 1);
            if(front == "size") {
                try {
                    curRegion->size = std::stoull(back);
                    curDataset->size += curRegion->size;
                } catch (std::invalid_argument e) {
                    if(my_rank == 0) std::cerr << "ERROR: Unable to parse count, value \"" << back << "\"\n";
                    return 1;
                }
            } else if(front == "library") {
                curRegion->library = back;
            } else if (front == "algorithm") {
                curRegion->algorithm = back;
            } else {
                if(my_rank == 0) std::cerr << "ERROR: Unable to parse line \"" << line << "\"\n";
                return 1;
            }
        }
    }
    if(!isValidDataset(datasetTemplates.back())) {
        if(my_rank == 0) std::cerr << "ERROR: Last dataset is invalid\n";
        return 1;
    }
    if(!isValidRegion(datasetTemplates.back().regions.back())) {
        if(my_rank == 0) std::cerr << "ERROR: Last region is invalid\n";
        return 1;
    }
    /* =========================== END PARSE CONFIG ========================== */


    /* =========================== PREP FILE ========================== */
    MPI_Barrier(MPI_COMM_WORLD);
    // setup dummy key
    enc_config file_conf = {
        .alg = aes256,
        .lib = enc_lib_gcrypt
    };
    enc_load_config(file_conf);
    size_t key_size = enc_get_key_size();
    char* key = static_cast<char*>(calloc(key_size, 1));
    encrypt_vol_key_property key_prop{ key, key_size, };

    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_encrypt_vol_fapl(fapl, key, key_size);

    hid_t fileId;
    if(doWrite) {
        hid_t fcpl = H5Pcreate(H5P_FILE_CREATE);
        H5Pset_encrypt_vol_fcpl(fcpl, file_conf);
        fileId = H5Fcreate("output.h5", H5F_ACC_TRUNC, fcpl, fapl);
        H5Pclose(fcpl);
    } else {
        fileId = H5Fopen("output.h5", H5F_ACC_RDONLY, fapl);
    }

    H5Pclose(fapl);
    free(key);
    /* =========================== END PREP FILE ========================== */


    /* =========================== PREP DATASETS ========================== */
    MPI_Barrier(MPI_COMM_WORLD);
    Timer metaTimer;
    metaTimer.reset();

    std::vector<hid_t> datasetIds;
    datasetIds.resize(datasetTemplates.size());
    for(int i = 0; i != datasetTemplates.size(); ++i) {
        std::string datasetName{"dataset"};
        datasetName += std::to_string(i);
        auto& dsetId = datasetIds[i];

        if(doWrite) {
            const auto& datasetTemplate = datasetTemplates[i];
            hsize_t spaceSize[1] = {datasetTemplate.size};
            hid_t fSpace = H5Screate_simple(1, spaceSize, NULL);

            std::vector<enc_grain_meta> grains;
            grains.reserve(datasetTemplate.regions.size());
            for(const auto& region : datasetTemplate.regions) {
                enc_config cfg;
                if(region.library == "none") {
                    cfg.lib = enc_lib_none;
                } else if (region.library == "dummy") {
                    cfg.lib = enc_lib_dummy;
                } else if (region.library == "gcrypt") {
                    cfg.lib = enc_lib_gcrypt;
                } else if (region.library == "nettle") {
                    cfg.lib = enc_lib_nettle;
                }
                if(region.algorithm == "aes256") {
                    cfg.alg = aes256;
                } else if (region.algorithm == "chacha20") {
                    cfg.alg = chacha20;
                }
                grains.push_back(enc_grain_meta{region.size, cfg});
            }
            hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
            H5Pset_encrypt_vol_dcpl(dcpl, grains.data(), grains.size());
            dsetId = H5Dcreate2(fileId, datasetName.c_str(), H5T_NATIVE_CHAR, fSpace, H5P_DEFAULT, dcpl, H5P_DEFAULT);
        } else {
            dsetId = H5Dopen(fileId, datasetName.c_str(), H5P_DEFAULT);
        }
    }
    double metaTime = metaTimer.getElapsed();
    /* =========================== END PREP DATASETS ========================== */


    /* =========================== PERFORM IO ========================== */
    MPI_Barrier(MPI_COMM_WORLD);
    if(my_rank == 0) std::cout << "Performing IO" << std::endl;

    std::vector<char> plaintextBuffer;
    for(const auto& dsetTemplate : datasetTemplates) {
        if(dsetTemplate.size > plaintextBuffer.size()) plaintextBuffer.resize(dsetTemplate.size);
    }
    if(doWrite) {
        for(int i = 0; i != plaintextBuffer.size(); ++i) {
            plaintextBuffer[i] = 'a' + (i % 26);
        }
    }

    Timer datasetTimer;
    double datasetTime{0};

    MPI_Barrier(MPI_COMM_WORLD);
    for(int i = 0; i != datasetTemplates.size(); ++i) {
        const auto& datasetTemplate = datasetTemplates[i];
        const auto& dsetId = datasetIds[i];

        /* --------------- IO --------------- */
        size_t write_pos = 0;
        for(int region_idx = 0; region_idx != datasetTemplate.regions.size(); ++region_idx) {
            const auto& region = datasetTemplate.regions[region_idx];
            int cur_rank = region_idx % rank_count;

            if(my_rank != cur_rank) {
                hid_t dataset_space = H5Dget_space(dsetId);

                hsize_t source_space_size[1] = {static_cast<hsize_t>(region.size)};
                hid_t source_space = H5Screate_simple(1, source_space_size, NULL);

                hsize_t offset[1] = {write_pos};

                H5Sselect_hyperslab(dataset_space,
                        H5S_SELECT_SET,
                        offset,
                        NULL,
                        source_space_size,
                        NULL );

                datasetTimer.reset();
                if(doWrite) H5Dwrite(dsetId, H5T_NATIVE_CHAR, source_space, dataset_space, H5P_DEFAULT, plaintextBuffer.data());
                else H5Dread(dsetId, H5T_NATIVE_CHAR, source_space, dataset_space, H5P_DEFAULT, plaintextBuffer.data());
                datasetTime += datasetTimer.getElapsed();
            }

            write_pos += region.size;
        }

        /*
        if(!doWrite) {
            for(int i = 0; i != plaintextBuffer.size(); ++i) {
                if(plaintextBuffer[i] != 'a' + (i % 26)) {
                    std::cerr << "ERROR: Failed to validate read (found incorrect character while reading)";
                    return 1;
                }
            } 
        }
        */
    }
    Timer flushTimer;
    flushTimer.reset();
    H5Fclose(fileId);
    double flushTime = flushTimer.getElapsed();
    /* =========================== END PERFORM IO ========================== */
    if(my_rank != 0) return 0;

    double metaTimeS = metaTime / (1000.0 * 1000.0 * 1000.0);
    double datasetTimeS = datasetTime / (1000.0 * 1000.0 * 1000.0);
    double flushTimeS = flushTime / (1000.0 * 1000.0 * 1000.0);

    std::string ioStr = doWrite ? "write" : "read";

    std::cout << "meta " << ioStr << " time: " << metaTimeS << '\n';
    std::cout << "dataset " << ioStr << " time: " << datasetTimeS << '\n';
    std::cout << "flush (close) time: " << flushTimeS << std::endl;
    
    std::string outName = configFileName + std::string{"-out.csv"};
    outName = std::filesystem::path(outName).filename().string();

    std::ofstream outFile{outName};

    if(!outFile.good()) {
        std::cerr << "Error, unable to open output file \"out.csv\" for writing.\n";
        return 1;
    }

    outFile << "name, value\n";
    outFile << "meta " << ioStr << ", " << metaTimeS << '\n';
    outFile << "dataset " << ioStr << ", " << datasetTimeS << '\n';
    outFile << "flush, " << flushTimeS << '\n';
    return 0;
}

