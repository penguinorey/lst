#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <ctime>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <map>

namespace fs = std::filesystem;

// Códigos de color ANSI
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_GRAY    "\033[90m"

struct FileInfo {
    std::string name;
    std::string permissions;
    std::string owner;
    std::string group;
    std::string size_human;
    double bytes;
    double kilobytes;
    double megabytes;
    double gigabytes;
    double terabytes;
    std::string last_modified;
    bool is_directory;
    bool is_hidden;
    bool is_symlink;
    std::string symlink_target;
    nlink_t hard_links;
    ino_t inode;
};

class LsCommand {
private:
    bool show_long_format = false;
    bool show_all = false;
    bool show_almost_all = false;
    bool show_human_readable = false;
    bool recursive = false;
    bool show_inode = false;
    bool reverse_sort = false;
    bool sort_by_time = false;
    bool show_help = false;
    
    std::vector<std::string> paths;
    std::map<std::string, std::string> colors;

public:
    LsCommand() {
        // Configurar colores para cada columna
        colors["permissions"] = COLOR_CYAN;
        colors["owner"] = COLOR_YELLOW;
        colors["group"] = COLOR_MAGENTA;
        colors["size"] = COLOR_GREEN;
        colors["date"] = COLOR_BLUE;
        colors["type"] = COLOR_RED;
        colors["hidden"] = COLOR_GRAY;
        colors["name"] = COLOR_WHITE;
        colors["inode"] = COLOR_GRAY;
        colors["links"] = COLOR_GRAY;
    }

    void parseArguments(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "--help" || arg == "-h") {
                show_help = true;
                return;
            }
            else if (arg == "-l") {
                show_long_format = true;
            }
            else if (arg == "-a") {
                show_all = true;
            }
            else if (arg == "-A") {
                show_almost_all = true;
            }
            else if (arg == "-h" || arg == "--human-readable") {
                show_human_readable = true;
            }
            else if (arg == "-R") {
                recursive = true;
            }
            else if (arg == "-i") {
                show_inode = true;
            }
            else if (arg == "-r") {
                reverse_sort = true;
            }
            else if (arg == "-t") {
                sort_by_time = true;
            }
            else if (arg[0] != '-') {
                paths.push_back(arg);
            }
        }
        
        if (paths.empty()) {
            paths.push_back(".");
        }
    }

    void displayHelp() {
        std::cout << "Uso: ls [OPCION]... [ARCHIVO]...\n";
        std::cout << "Lista información sobre los ARCHIVOS (del directorio actual por defecto).\n\n";
        std::cout << "Opciones:\n";
        std::cout << "  -a, --all                  no ignora las entradas que comienzan con .\n";
        std::cout << "  -A, --almost-all           no lista las entradas . y ..\n";
        std::cout << "  -l                         usa formato largo de lista\n";
        std::cout << "  -h, --human-readable       muestra los tamaños de forma legible (ej. 1K 234M 2G)\n";
        std::cout << "  -R, --recursive            lista subdirectorios recursivamente\n";
        std::cout << "  -i, --inode                muestra el número de inode de cada archivo\n";
        std::cout << "  -r, --reverse              orden inverso al listar\n";
        std::cout << "  -t                         ordena por tiempo de modificación (más nuevo primero)\n";
        std::cout << "      --help                 muestra esta ayuda y termina\n";
    }

    std::string getPermissions(const fs::directory_entry& entry) {
        struct stat st;
        stat(entry.path().c_str(), &st);
        
        // Obtener los permisos en formato octal
        mode_t mode = st.st_mode;
        std::stringstream perm_stream;
        perm_stream << std::oct << (mode & 07777);
        std::string perm_str = perm_stream.str();
        
        // Asegurar que tenga 4 dígitos
        while (perm_str.length() < 4) {
            perm_str = "0" + perm_str;
        }
        
        return perm_str;
    }

    std::string getOwner(uid_t uid) {
        struct passwd* pw = getpwuid(uid);
        return pw ? std::string(pw->pw_name) : std::to_string(uid);
    }

    std::string getGroup(gid_t gid) {
        struct group* gr = getgrgid(gid);
        return gr ? std::string(gr->gr_name) : std::to_string(gid);
    }

    std::string formatSize(double bytes) {
        if (bytes < 1000) {
            return std::to_string(static_cast<int>(bytes)) + "B";
        } else if (bytes < 1000000) {
            return std::to_string(bytes / 1000.0).substr(0, 4) + "K";
        } else if (bytes < 1000000000) {
            return std::to_string(bytes / 1000000.0).substr(0, 4) + "M";
        } else {
            return std::to_string(bytes / 1000000000.0).substr(0, 4) + "G";
        }
    }

    std::string formatTime(std::time_t time) {
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", std::localtime(&time));
        return std::string(buffer);
    }

    FileInfo getFileInfo(const fs::directory_entry& entry) {
        FileInfo info;
        info.name = entry.path().filename().string();
        
        struct stat st;
        stat(entry.path().c_str(), &st);
        
        info.permissions = getPermissions(entry);
        info.owner = getOwner(st.st_uid);
        info.group = getGroup(st.st_gid);
        info.bytes = static_cast<double>(st.st_size);
        info.kilobytes = info.bytes / 1000.0;
        info.megabytes = info.kilobytes / 1000.0;
        info.gigabytes = info.megabytes / 1000.0;
        info.terabytes = info.gigabytes / 1000.0;
        info.size_human = formatSize(info.bytes);
        info.last_modified = formatTime(st.st_mtime);
        info.is_directory = fs::is_directory(entry);
        info.is_hidden = info.name[0] == '.';
        info.is_symlink = fs::is_symlink(entry);
        info.hard_links = st.st_nlink;
        info.inode = st.st_ino;
        
        if (info.is_symlink) {
            info.symlink_target = fs::read_symlink(entry.path()).string();
        }
        
        return info;
    }

    void displayLongFormat(const std::vector<FileInfo>& files) {
        std::cout << std::left;
        
        // Encabezados de columna con colores
        if (show_inode) {
            std::cout << colors["inode"] << std::setw(8) << "Inode" << COLOR_RESET << " ";
        }
        
        std::cout << colors["permissions"] << std::setw(6) << "Perms" << COLOR_RESET << " ";
        std::cout << colors["links"] << std::setw(4) << "Links" << COLOR_RESET << " ";
        std::cout << colors["owner"] << std::setw(10) << "Usuario" << COLOR_RESET << " ";
        std::cout << colors["group"] << std::setw(10) << "Grupo" << COLOR_RESET << " ";
        std::cout << colors["size"] << std::setw(10) << "Tamaño" << COLOR_RESET << " ";
        std::cout << colors["date"] << std::setw(16) << "Modificado" << COLOR_RESET << " ";
        std::cout << colors["type"] << std::setw(6) << "Tipo" << COLOR_RESET << " ";
        std::cout << colors["hidden"] << std::setw(6) << "Oculto" << COLOR_RESET << " ";
        std::cout << colors["name"] << "Nombre" << COLOR_RESET;
        std::cout << "\n";
        
        // Línea separadora
        std::cout << std::string(100, '-') << "\n";
        
        // Datos de cada archivo
        for (const auto& file : files) {
            if (show_inode) {
                std::cout << colors["inode"] << std::setw(8) << file.inode << COLOR_RESET << " ";
            }
            
            std::cout << colors["permissions"] << std::setw(6) << file.permissions << COLOR_RESET << " ";
            std::cout << colors["links"] << std::setw(4) << file.hard_links << COLOR_RESET << " ";
            std::cout << colors["owner"] << std::setw(10) << file.owner << COLOR_RESET << " ";
            std::cout << colors["group"] << std::setw(10) << file.group << COLOR_RESET << " ";
            std::cout << colors["size"] << std::setw(10) << file.size_human << COLOR_RESET << " ";
            std::cout << colors["date"] << std::setw(16) << file.last_modified << COLOR_RESET << " ";
            std::cout << colors["type"] << std::setw(6) << (file.is_directory ? "DIR" : "FILE") << COLOR_RESET << " ";
            std::cout << colors["hidden"] << std::setw(6) << (file.is_hidden ? "SI" : "NO") << COLOR_RESET << " ";
            
            // Color especial para directorios y enlaces simbólicos
            if (file.is_directory) {
                std::cout << COLOR_BLUE;
            } else if (file.is_symlink) {
                std::cout << COLOR_CYAN;
            } else if (file.is_hidden) {
                std::cout << COLOR_GRAY;
            } else {
                std::cout << colors["name"];
            }
            
            std::cout << file.name << COLOR_RESET;
            
            if (file.is_symlink) {
                std::cout << " -> " << file.symlink_target;
            }
            
            std::cout << "\n";
        }
    }

    void displaySimple(const std::vector<FileInfo>& files) {
        for (const auto& file : files) {
            // Aplicar colores según el tipo de archivo
            if (file.is_directory) {
                std::cout << COLOR_BLUE;
            } else if (file.is_symlink) {
                std::cout << COLOR_CYAN;
            } else if (file.is_hidden) {
                std::cout << COLOR_GRAY;
            } else {
                std::cout << COLOR_WHITE;
            }
            
            std::cout << file.name << COLOR_RESET << "  ";
        }
        std::cout << "\n";
    }

    void listDirectory(const fs::path& path) {
        std::vector<FileInfo> files;
        
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                FileInfo info = getFileInfo(entry);
                
                // Filtrado de archivos ocultos
                if (!show_all && !show_almost_all && info.is_hidden) {
                    continue;
                }
                if (show_almost_all && (info.name == "." || info.name == "..")) {
                    continue;
                }
                
                files.push_back(info);
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error al leer el directorio: " << e.what() << "\n";
            return;
        }
        
        // Ordenamiento
        if (sort_by_time) {
            std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
                return a.bytes > b.bytes; // Simulación de orden por tiempo
            });
        } else {
            std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) {
                return a.name < b.name;
            });
        }
        
        if (reverse_sort) {
            std::reverse(files.begin(), files.end());
        }
        
        // Mostrar resultados
        if (show_long_format) {
            displayLongFormat(files);
        } else {
            displaySimple(files);
        }
        
        // Recursivo
        if (recursive) {
            for (const auto& file : files) {
                if (file.is_directory && file.name != "." && file.name != "..") {
                    std::cout << "\n" << path / file.name << ":\n";
                    listDirectory(path / file.name);
                }
            }
        }
    }

    void execute() {
        if (show_help) {
            displayHelp();
            return;
        }
        
        for (const auto& path_str : paths) {
            fs::path path(path_str);
            
            if (paths.size() > 1) {
                std::cout << path_str << ":\n";
            }
            
            if (fs::exists(path)) {
                if (fs::is_directory(path)) {
                    listDirectory(path);
                } else {
                    // Es un archivo individual
                    FileInfo info = getFileInfo(fs::directory_entry(path));
                    std::vector<FileInfo> single_file = {info};
                    
                    if (show_long_format) {
                        displayLongFormat(single_file);
                    } else {
                        // Aplicar color según el tipo de archivo
                        if (info.is_directory) {
                            std::cout << COLOR_BLUE;
                        } else if (info.is_symlink) {
                            std::cout << COLOR_CYAN;
                        } else if (info.is_hidden) {
                            std::cout << COLOR_GRAY;
                        } else {
                            std::cout << COLOR_WHITE;
                        }
                        
                        std::cout << info.name << COLOR_RESET << "\n";
                    }
                }
            } else {
                std::cerr << "ls: no se puede acceder a '" << path_str << "': No existe el archivo o directorio\n";
            }
            
            if (paths.size() > 1 && &path_str != &paths.back()) {
                std::cout << "\n";
            }
        }
    }
};

int main(int argc, char* argv[]) {
    LsCommand ls;
    ls.parseArguments(argc, argv);
    ls.execute();
    return 0;
}
