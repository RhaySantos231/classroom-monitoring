// server.cpp
// Compile: Visual Studio C++ (add Ws2_32.lib). C++17
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <string>

#pragma comment(lib, "Ws2_32.lib")
constexpr int PORT = 60000;
constexpr int BUFFER_SIZE = 4096;

struct ClientInfo {
    SOCKET sock;
    std::string name; // hostname or identifier
    std::string ip;
};

std::mutex clients_mutex;
std::unordered_map<int, ClientInfo> clients; // key = client id
int next_client_id = 1;

void send_line(SOCKET s, const std::string& line) {
    std::string msg = line + "\n";
    send(s, msg.c_str(), (int)msg.size(), 0);
}

void client_thread(int client_id) {
    ClientInfo ci;
    {
        std::lock_guard<std::mutex> lg(clients_mutex);
        ci = clients[client_id];
    }

    char buf[BUFFER_SIZE];
    while (true) {
        int r = recv(ci.sock, buf, BUFFER_SIZE - 1, 0);
        if (r <= 0) break;
        buf[r] = '\0';
        std::string msg(buf);
        // Trim newline
        if (!msg.empty() && msg.back() == '\n') msg.pop_back();
        // Example: handle heartbeat or status messages
        std::cout << "[msg from " << client_id << " - " << ci.name << "] " << msg << "\n";
    }

    // disconnect
    closesocket(ci.sock);
    {
        std::lock_guard<std::mutex> lg(clients_mutex);
        clients.erase(client_id);
    }
    std::cout << "[client " << client_id << " disconnected]\n";
}

void accept_thread(SOCKET listen_sock) {
    while (true) {
        sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        SOCKET client_sock = accept(listen_sock, (sockaddr*)&client_addr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            std::cerr << "accept failed\n";
            continue;
        }
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, sizeof(ipstr));
        // Expect first message from client to be IDENT:<name>
        char buf[BUFFER_SIZE];
        int r = recv(client_sock, buf, BUFFER_SIZE - 1, 0);
        if (r <= 0) {
            closesocket(client_sock);
            continue;
        }
        buf[r] = '\0';
        std::string first(buf);
        if (!first.empty() && first.back() == '\n') first.pop_back();
        std::string name = first;
        int id;
        {
            std::lock_guard<std::mutex> lg(clients_mutex);
            id = next_client_id++;
            clients[id] = ClientInfo{client_sock, name, ipstr};
        }
        std::cout << "[new client " << id << " - " << name << " @ " << ipstr << "]\n";
        // start handler thread
        std::thread t(client_thread, id);
        t.detach();
    }
}

void send_command_to_client(int client_id, const std::string& cmd) {
    std::lock_guard<std::mutex> lg(clients_mutex);
    auto it = clients.find(client_id);
    if (it == clients.end()) {
        std::cout << "Cliente não encontrado\n";
        return;
    }
    send_line(it->second.sock, cmd);
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(listen_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(listen_sock, SOMAXCONN);
    std::cout << "Servidor escutando na porta " << PORT << "\n";

    std::thread acc(accept_thread, listen_sock);
    acc.detach();

    // Simple console UI
    while (true) {
        std::cout << "\nComandos: list | send <id> <command> | broadcast <command> | exit\n> ";
        std::string op;
        std::cin >> op;
        if (op == "list") {
            std::lock_guard<std::mutex> lg(clients_mutex);
            if (clients.empty()) std::cout << "Nenhum cliente conectado\n";
            for (auto &p : clients) {
                std::cout << "ID: " << p.first << " - " << p.second.name << " (" << p.second.ip << ")\n";
            }
        } else if (op == "send") {
            int id;
            std::cin >> id;
            std::string rest;
            std::getline(std::cin, rest); // pega o resto da linha
            if (!rest.empty() && rest.front() == ' ') rest.erase(0,1);
            send_command_to_client(id, rest);
        } else if (op == "broadcast") {
            std::string rest;
            std::getline(std::cin, rest);
            if (!rest.empty() && rest.front() == ' ') rest.erase(0,1);
            std::lock_guard<std::mutex> lg(clients_mutex);
            for (auto &p : clients) send_line(p.second.sock, rest);
        } else if (op == "exit") {
            break;
        } else {
            std::cout << "Comando desconhecido\n";
        }
    }

    closesocket(listen_sock);
    WSACleanup();
    return 0;
}
