#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <sstream>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// Constants
const size_t TIFF_MAX_SIZE = 100 * 1024; // 100 KB for TIFF heuristic
const std::string RED = "\033[91m";
const std::string RESET = "\033[0m";

// File signature structure
struct Signature {
    std::vector<uint8_t> start;
    std::vector<uint8_t> end;
};

// File signatures
const std::unordered_map<std::string, Signature> FILE_SIGNATURES = {
    {"jpg", {{0xFF, 0xD8}, {0xFF, 0xD9}}},
    {"png", {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, {0x49, 0x45, 0x4E, 0x44}}},
    {"gif", {{0x47, 0x49, 0x46, 0x38, 0x37, 0x61}, {0x3B}}}, // Also matches GIF89a
    {"bmp", {{0x42, 0x4D}, {}}}, // No end signature
    {"tiff", {{0x49, 0x49, 0x2A, 0x00}, {}}}, // Also matches MM\x00\x2A
    {"pdf", {{0x25, 0x50, 0x44, 0x46, 0x2D}, {0x25, 0x25, 0x45, 0x4F, 0x46}}},
    {"zip", {{0x50, 0x4B, 0x03, 0x04}, {}}}
};

// Platform-specific memory mapping
struct MemoryMappedFile {
    uint8_t* data;
    size_t size;
#ifdef _WIN32
    HANDLE file;
    HANDLE mapping;
#else
    int fd;
#endif

    MemoryMappedFile(const std::string& path) {
        size = 0;
        data = nullptr;
#ifdef _WIN32
        file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        LARGE_INTEGER file_size;
        GetFileSizeEx(file, &file_size);
        size = static_cast<size_t>(file_size.QuadPart);
        mapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping) {
            CloseHandle(file);
            throw std::runtime_error("Failed to create file mapping");
        }
        data = static_cast<uint8_t*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
        if (!data) {
            CloseHandle(mapping);
            CloseHandle(file);
            throw std::runtime_error("Failed to map file");
        }
#else
        fd = open(path.c_str(), O_RDONLY);
        if (fd == -1) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        struct stat sb;
        if (fstat(fd, &sb) == -1) {
            close(fd);
            throw std::runtime_error("Failed to get file size");
        }
        size = static_cast<size_t>(sb.st_size);
        data = static_cast<uint8_t*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
        if (data == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("Failed to map file");
        }
#endif
    }

    ~MemoryMappedFile() {
#ifdef _WIN32
        if (data) UnmapViewOfFile(data);
        if (mapping) CloseHandle(mapping);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
#else
        if (data && data != MAP_FAILED) munmap(data, size);
        if (fd != -1) close(fd);
#endif
    }
};

// Platform-specific directory creation
void create_directory(const std::string& path) {
#ifdef _WIN32
    CreateDirectoryA(path.c_str(), nullptr);
#else
    mkdir(path.c_str(), 0755);
#endif
}

void print_banner() {
    const std::string banner = R"(
888b. 888b.              88888
|   | |   |                |
|   8 8wwwP .d88b  Yb dP   8   
8   8 |   | |____|  `8.    8   
888P' 888P' `Y88P  dP Yb   8   
)";
    std::cout << banner << std::endl;
}

std::string get_unique_folder_name(const std::string& base_name) {
    std::string folder_name = base_name;
    int counter = 1;
    while (std::ifstream(folder_name)) {
        folder_name = base_name + "_" + std::to_string(counter);
        counter++;
    }
    return folder_name;
}

void save_file(const uint8_t* data, size_t size, const std::string& ext, int count, const std::string& output_folder) {
    std::string folder_name = output_folder + "/" + ext;
    create_directory(folder_name);
    std::string file_path = folder_name + "/extracted_" + std::to_string(count) + "." + ext;
    std::ofstream out(file_path, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to write " << file_path << std::endl;
        return;
    }
    out.write(reinterpret_cast<const char*>(data), size);
    out.close();
    std::cout << "[+] Extracted " << file_path << std::endl;
}

std::pair<size_t, std::string> find_next_signature(const uint8_t* data, size_t size, size_t pos, const std::set<std::string>& formats) {
    size_t min_pos = size;
    std::string found_fmt;
    for (const auto& fmt : formats) {
        auto it = FILE_SIGNATURES.find(fmt);
        if (it == FILE_SIGNATURES.end()) continue;
        const auto& sig = it->second.start;
        auto result = std::search(data + pos, data + size, sig.begin(), sig.end());
        size_t match_pos = result - data;
        if (match_pos < min_pos) {
            min_pos = match_pos;
            found_fmt = fmt;
        }
    }
    return (found_fmt.empty() || min_pos >= size) ? std::make_pair(size, "") : std::make_pair(min_pos, found_fmt);
}

void extract_files_from_db(const std::string& file_path, const std::set<std::string>& selected_formats, const std::string& output_base_folder) {
    std::unordered_map<std::string, int> count_dict;
    try {
        MemoryMappedFile mm(file_path);
        size_t pos = 0;
        while (pos < mm.size) {
            auto [start, fmt] = find_next_signature(mm.data, mm.size, pos, selected_formats);
            if (fmt.empty()) break;

            size_t end = start + 1;
            if (fmt == "jpg") {
                auto end_it = std::search(mm.data + start, mm.data + mm.size, 
                                         FILE_SIGNATURES.at("jpg").end.begin(), 
                                         FILE_SIGNATURES.at("jpg").end.end());
                end = (end_it != mm.data + mm.size) ? end_it - mm.data + 2 : start + 2;
            } else if (fmt == "png") {
                auto end_it = std::search(mm.data + start, mm.data + mm.size, 
                                         FILE_SIGNATURES.at("png").end.begin(), 
                                         FILE_SIGNATURES.at("png").end.end());
                end = (end_it != mm.data + mm.size) ? end_it - mm.data + 12 : start + 12;
            } else if (fmt == "gif") {
                auto end_it = std::search(mm.data + start, mm.data + mm.size, 
                                         FILE_SIGNATURES.at("gif").end.begin(), 
                                         FILE_SIGNATURES.at("gif").end.end());
                end = (end_it != mm.data + mm.size) ? end_it - mm.data + 1 : start + 1;
            } else if (fmt == "bmp") {
                if (start + 6 <= mm.size) {
                    uint32_t size = *reinterpret_cast<const uint32_t*>(mm.data + start + 2);
                    if (size > 0 && start + size <= mm.size) {
                        end = start + size;
                    } else {
                        end = start + 2;
                    }
                }
            } else if (fmt == "tiff") {
                end = std::min(start + TIFF_MAX_SIZE, mm.size);
            } else if (fmt == "pdf") {
                auto end_it = std::search(mm.data + start, mm.data + mm.size, 
                                         FILE_SIGNATURES.at("pdf").end.begin(), 
                                         FILE_SIGNATURES.at("pdf").end.end());
                end = (end_it != mm.data + mm.size) ? end_it - mm.data + FILE_SIGNATURES.at("pdf").end.size() : start + 6;
            } else if (fmt == "zip") {
                auto end_it = std::search(mm.data + start + 4, mm.data + mm.size, 
                                         FILE_SIGNATURES.at("zip").start.begin(), 
                                         FILE_SIGNATURES.at("zip").start.end());
                end = (end_it != mm.data + mm.size) ? end_it - mm.data : mm.size;
            }

            save_file(mm.data + start, end - start, fmt, count_dict[fmt]++, output_base_folder);
            pos = end;
        }
    } catch (const std::exception& e) {
        std::cerr << "Extraction error: " << e.what() << std::endl;
        throw;
    }
}

int main() {
    print_banner();

    const std::vector<std::string> file_types = {"jpg", "png", "gif", "bmp", "tiff", "pdf", "zip"};
    
    std::cout << RED << "Please place your file in the tool path" << RESET << "\n\n";
    std::cout << "Select file type(s) to extract (comma separated numbers):\n";
    std::cout << "0. ALL supported types\n";
    for (size_t i = 0; i < file_types.size(); ++i) {
        std::string upper = file_types[i];
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        std::cout << i + 1 << ". " << upper << "\n";
    }

    std::set<std::string> selected_formats;
    while (true) {
        std::cout << "Your choice: ";
        std::string choice;
        std::getline(std::cin, choice);
        choice.erase(std::remove_if(choice.begin(), choice.end(), ::isspace), choice.end());

        if (choice == "0") {
            selected_formats.insert(file_types.begin(), file_types.end());
            break;
        }

        bool valid = true;
        std::set<std::string> formats;
        std::stringstream ss(choice);
        std::string item;
        while (std::getline(ss, item, ',')) {
            try {
                size_t idx = std::stoul(item) - 1;
                if (idx < file_types.size()) {
                    formats.insert(file_types[idx]);
                } else {
                    valid = false;
                }
            } catch (...) {
                valid = false;
            }
        }

        if (valid && !formats.empty()) {
            selected_formats = formats;
            break;
        }
        std::cout << "Invalid choice, please enter valid number(s) from the list.\n";
    }

    std::cout << "\nEnter the input filename: ";
    std::string file_path;
    std::getline(std::cin, file_path);
    file_path.erase(std::remove_if(file_path.begin(), file_path.end(), ::isspace), file_path.end());

    std::ifstream file_check(file_path, std::ios::binary);
    if (!file_check) {
        std::cout << RED << "File '" << file_path << "' not found." << RESET << "\n";
        return 1;
    }
    file_check.close();

    std::string base_name = file_path.substr(0, file_path.find_last_of("."));
    std::string output_base_folder = get_unique_folder_name(base_name);
    create_directory(output_base_folder);

    std::cout << "\nExtracted files will be saved under: '" << output_base_folder << "'\n\n";

    try {
        extract_files_from_db(file_path, selected_formats, output_base_folder);
    } catch (...) {
        return 1;
    }

    return 0;
}