#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>

#include <cstring>

#include <hdf5.h>

extern "C" {
#include "encryption_wrapper/enc_wrapper.h"
#include "encryption_wrapper/enc_algorithm.h"
#include "encryption_wrapper/gcrypt_impl/enc_gcrypt.h"
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

struct Dataset {
    size_t count;
    std::string algorithm;
    std::string library;
};

static inline bool isValidDataset(const Dataset& dataset) {
    return dataset.count > 0 &&
        (dataset.algorithm == "aes256" || dataset.algorithm == "chacha20" || dataset.algorithm == "none") &&
        (dataset.library == "nettle" || dataset.library == "gcrypt" || dataset.library == "none");
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cout << "Incorrect usage.\n";
        std::cout << "Usage: " << argv[0] << " <config>\n";
        return 1;
    }

    std::string configFileName{argv[1]};
    std::ifstream configFile{configFileName};

    if(!configFile.good()) {
        std::cerr << "ERROR: Unable to open file \"" << configFileName << "\"\n";
        return 1;
    }

    /* =========================== PARSE CONFIG ========================== */
    std::vector<Dataset> datasetTemplates;
    Dataset* curDataset = nullptr;
    std::string line;
    while(std::getline(configFile, line)) {
        // check for beginning of dataset
        if(line == "dataset") {
            // check if current dataset is valid (specified all values)
            if(curDataset != nullptr && !isValidDataset(*curDataset)) {
                // error out because we didn't read a full dataset
                std::cerr << "ERROR: Invalid dataset description in config file\n";
                return 1;
            }

            // generate new dataset
            datasetTemplates.emplace_back(Dataset{});
            curDataset = &datasetTemplates.back();
        }

        // fill in dataset values
        else {
            if(curDataset == nullptr) {
                std::cerr << "ERROR: Initial dataset hasn't been started (did you forget \"dataset\" at the beginning of the file?\n";
                return 1;
            }
            // split line at = sign
            auto splitPos = line.find('=');
            if(splitPos == std::string::npos || splitPos == line.size() - 1) {
                std::cerr << "ERROR: Unable to parse line \"" << line << "\"\n";
                return 1;
            }

            std::string front = line.substr(0, splitPos);
            std::string back = line.substr(splitPos + 1);
            if(front == "count") {
                try {
                    curDataset->count = std::stoull(back);
                } catch (std::invalid_argument e) {
                    std::cerr << "ERROR: Unable to parse count, value \"" << back << "\"\n";
                    return 1;
                }
            } else if(front == "library") {
                curDataset->library = back;
            } else if (front == "algorithm") {
                curDataset->algorithm = back;
            } else {
                std::cerr << "ERROR: Unable to parse line \"" << line << "\"\n";
                return 1;
            }
        }
    }
    if(!isValidDataset(datasetTemplates.back())) {
        // error out because we didn't read a full dataset
        std::cerr << "ERROR: Invalid dataset description in config file\n";
        return 1;
    }
    /* =========================== END PARSE CONFIG ========================== */


    /* =========================== PREP FILE ========================== */
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    hid_t fileId = H5Fcreate("output.h5", H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
    H5Pclose(fapl);
    /* =========================== END PREP FILE ========================== */


    /* =========================== PREP DATASETS ========================== */
    Timer metaTimer;
    metaTimer.reset();
    // setup dummy key
    enc_load_library(enc_get_gcrypt());
    enc_prepare(aes256);
    size_t key_size = enc_get_key_size();
    char* key = static_cast<char*>(calloc(key_size, 1));

    // setup template encryption properties
    encrypt_vol_property enc_prop {
        -1          // alg
    };
    encrypt_vol_key_property enc_key_prop{
        key,        // key
        key_size    // key size
    };

    std::vector<hid_t> datasetIds;
    datasetIds.resize(datasetTemplates.size());
    for(int i = 0; i != datasetTemplates.size(); ++i) {
        const auto& datasetTemplate = datasetTemplates[i];
        auto& dsetId = datasetIds[i];
        std::string datasetName{"dataset"};
        datasetName += std::to_string(i);
        hsize_t spaceSize[1] = {datasetTemplate.count};
        hid_t fSpace = H5Screate_simple(1, spaceSize, NULL);

        if(datasetTemplate.algorithm == "none") {
            enc_prop.alg = -1;
        }
        else if(datasetTemplate.algorithm == "aes256") {
            enc_prop.alg = aes256;
        } else if (datasetTemplate.algorithm == "chacha20") {
            enc_prop.alg = chacha20;
        }

        hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
        hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
        H5Pset(dcpl, ENCRYPT_VOL_PROPERTY_NAME, &enc_prop);
        H5Pset(dapl, ENCRYPT_VOL_KEY_PROPERTY_NAME, &enc_key_prop);

        dsetId = H5Dcreate2(fileId, datasetName.c_str(), H5T_NATIVE_INT, fSpace, H5P_DEFAULT, dcpl, dapl);
    }
    double metaTime = metaTimer.getElapsed();
    /* =========================== END PREP DATASETS ========================== */


    /* =========================== PERFORM IO ========================== */
    Timer writeTimer;
    Timer datasetTimer;
    double datasetTime{0};
    writeTimer.reset();

    // re-use buffer
    std::vector<char> plaintextBuffer;
    for(int i = 0; i != datasetTemplates.size(); ++i) {
        const auto& datasetTemplate = datasetTemplates[i];
        const auto& dsetId = datasetIds[i];

        const std::size_t ioCount = datasetTemplate.count;
        const std::size_t ioSize = ioCount * SHARED_BLOCK_SIZE;

        // allocate a buffers
        plaintextBuffer.resize(ioSize);

        /* --------------- IO --------------- */
        datasetTimer.reset();
        H5Dwrite(dsetId, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, plaintextBuffer.data());
        datasetTime += datasetTimer.getElapsed();
    }
    Timer flushTimer;
    flushTimer.reset();
    H5Fclose(fileId);
    double flushTime = flushTimer.getElapsed();
    double writeTime = writeTimer.getElapsed();
    /* =========================== END PERFORM IO ========================== */

    double writeTimeS = writeTime / (1000.0 * 1000.0 * 1000.0);
    double metaTimeS = metaTime / (1000.0 * 1000.0 * 1000.0);
    double datasetTimeS = datasetTime / (1000.0 * 1000.0 * 1000.0);
    double flushTimeS = flushTime / (1000.0 * 1000.0 * 1000.0);

    std::cout << "write time: " << writeTimeS << '\n';
    std::cout << "meta write time: " << metaTimeS << '\n';
    std::cout << "dataset write time: " << datasetTimeS << '\n';
    std::cout << "flush (close) time: " << flushTimeS << '\n';
    
    std::string outName = configFileName + std::string{"-out.csv"};
    std::ofstream outFile{outName};

    if(!outFile.good()) {
        std::cerr << "Error, unable to open output file \"out.csv\" for writing.\n";
        return 1;
    }

    outFile << "name, value\n";
    outFile << "write, " << writeTimeS << '\n';
    outFile << "meta write, " << metaTimeS << '\n';
    outFile << "dataset write, " << datasetTimeS << '\n';
    outFile << "flush, " << flushTimeS << '\n';
    return 0;
}

