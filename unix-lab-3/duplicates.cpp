#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <openssl/sha.h>

namespace fs = std::filesystem;

std::string sha1sum(const fs::path& path) 
{
    std::ifstream file(path);
    if (!file) return "";

    SHA_CTX sha_ctx;
    SHA1_Init(&sha_ctx);

    char buffer[512];
    // Пока успешное чтение по 512 бит или ненулевое прочитанное количество символов в предыдущей операции чтения (чтобы остаток после последних 512 учитывать)
    while(file.read(buffer, sizeof(buffer)) || file.gcount())
        SHA1_Update(&sha_ctx, buffer, file.gcount());

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &sha_ctx);

    std::ostringstream result;
    for (unsigned char c : hash)
        result << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    std::cout << path << " hash is " << result.str() << "\n";

    return result.str();
}

int main(int argc, char* argv[]){
    if (argc < 2) {
        std::cerr << "Usage: ./duplicates <directory>\n";
        return 1;
    }
    fs::path root_dir_path = argv[1];
    // хэш - путь
    std::unordered_map<std::string, fs::path> seen_files;

    // fs_object - папка/файл/symlink...
    for (auto& fs_object : fs::recursive_directory_iterator(root_dir_path)){
        if (!fs::is_regular_file(fs_object.path())) continue;

        std::string hash = sha1sum(fs_object.path());
        if (hash.empty()) continue;

        // если такой хэш уже встречали
        if (seen_files.count(hash)){
            fs::path seen_file_path = seen_files[hash];
            // Если fs_object - уже жесткая ссылка на seen_file
            if (fs::equivalent(seen_file_path, fs_object.path())){
                std::cout << "Files " << seen_file_path << " and " << fs_object.path() << "are already hard-linked.\n";
            }
            else{
                // Файл-дубликат fs_object заменяется жёсткой ссылкой на файл с таким же хэшем, который мы встречали ранее 
                fs::remove(fs_object.path());
                fs::create_hard_link(seen_file_path, fs_object.path());
                std::cout << "Replaced " << fs_object.path() << " with hard link to " << seen_file_path << "\n";
            }
        }
        else{
            seen_files[hash] = fs_object.path();
        }
    }
    return 0;
}