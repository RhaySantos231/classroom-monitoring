// client.cpp
// Compile: Visual Studio C++ (add Ws2_32.lib). C++17
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")
constexpr char SERVER_IP[] = "192.168.0.10"; // <-- coloque o IP do servidor
constexpr int SERVER_PORT = 60000;
constexpr int BUFFER_SIZE = 4096;

std::atomic<bool> running{true};

// função util: manda linha com \n
void send_line(SOCKET s, const std::string& line) {
    std::string msg = line + "\n";
    send(s, msg.c_str(), (int)msg.size(), 0);
}

// Executa comandos simples recebidos do servidor
void handle_command(const std::string& cmd) {
    // Exemplos de comandos:
    // KILL process.exe
    // SHUTDOWN
    // RESTART
    // CUSTOM some-shell-command
    std::istringstream iss(cmd);
    std::string op;
    iss >> op;
    if (op == "KILL") {
        std::string proc;
        iss >> proc;
        if (!proc.empty()) {
            std::string syscmd = "taskkill /IM " + proc + " /F";
            system(syscmd.c_str()); // precisa ser admin para matar alguns processos
        }
    } else if (op == "SHUTDOWN") {
        system("shutdown /s /t 5");
    } else if (op == "RESTART") {
        system("shutdown /r /t 5");
    } else if (op == "CUSTOM") {
        std::string rest;
        std::getline(iss, rest);
        if (!rest.empty() && rest.front() == ' ') rest.erase(0,1);
        system(rest.c_str());
    } else {
        // comando desconhecido: trate conforme precisar
        std::cout << "Comando desconhecido: " << cmd << "\n";
    }
}

// Monitor simples: integra com seu sistema de bloqueio.
// Aqui só um exemplo que tenta matar um processo proibido periodicamente.
void monitor_blocked_apps() {
    // Se você já tem esta função no projeto, substitua o conteúdo por chamada à sua rotina.
    while (running) {
        // Exemplo: se quiser bloquear "notepad.exe"
        // system("taskkill /IM notepad.exe /F");
        Sleep(2000);
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &srv.sin_addr);
    srv.sin_port = htons(SERVER_PORT);

    if (connect(sock, (sockaddr*)&srv, sizeof(srv)) == SOCKET_ERROR) {
        std::cerr << "Falha ao conectar ao servidor\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // Envia identificação: HOSTNAME
    char hostname[256];
    GetComputerNameA(hostname, (LPDWORD) (LPVOID) (new DWORD(256))); // simplificação
    // safer: use GetComputerNameA with proper buffer; for brevidade:
    std::string id = hostname;
    if (id.empty()) id = "cliente_desconhecido";
    send_line(sock, id);

    // Start monitor thread (integre seu bloqueador aqui)
    std::thread monitorThread(monitor_blocked_apps);

    // Receiver loop
    char buf[BUFFER_SIZE];
    while (true) {
        int r = recv(sock, buf, BUFFER_SIZE - 1, 0);
        if (r <= 0) break;
        buf[r] = '\0';
        std::string msg(buf);
        if (!msg.empty() && msg.back() == '\n') msg.pop_back();
        std::cout << "[comando do servidor] " << msg << "\n";
        handle_command(msg);
    }

    running = false;
    monitorThread.join();
    closesocket(sock);
    WSACleanup();
    return 0;
}
