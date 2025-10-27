// identifica_origem_delete.cpp
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

using namespace std;

string WideToString(const wstring& w) {
    if (w.empty()) return string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    string s(size_needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], size_needed, nullptr, nullptr);
    return s;
}

// Obtém caminho completo do processo por PID
bool GetProcessImagePath(DWORD pid, string &outPath) {
    outPath.clear();
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    WCHAR buf[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(h, 0, buf, &size)) {
        outPath = WideToString(wstring(buf, buf + size));
        CloseHandle(h);
        return true;
    }
    CloseHandle(h);
    return false;
}

// Encerra o processo
bool TerminateProcessByPID(DWORD pid) {
    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProc) return false;
    bool result = TerminateProcess(hProc, 0) != 0;
    CloseHandle(hProc);
    return result;
}

// Tenta apagar o arquivo (.exe)
bool DeleteExecutable(const string& path) {
    // Remove atributos somente leitura
    SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    if (DeleteFileA(path.c_str())) {
        cout << "  Arquivo excluido com sucesso: " << path << "\n";
        return true;
    } else {
        cout << "  Falha ao excluir arquivo: " << path << " | Erro: " << GetLastError() << "\n";
        return false;
    }
}

// Função principal: verifica processos e tenta encerrar/apagar
void verificarEBloquear() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W processo;
    processo.dwSize = sizeof(PROCESSENTRY32W);

    vector<string> programasBloqueados = {
        "notepad.exe",
        "roblox.exe",
        "discord.exe",
        "minecraft.exe"
    };

    if (Process32FirstW(snapshot, &processo)) {
        do {
            string nome = WideToString(wstring(processo.szExeFile));
            for (const string& bloqueado : programasBloqueados) {
                if (_stricmp(nome.c_str(), bloqueado.c_str()) == 0) {
                    DWORD pid = processo.th32ProcessID;
                    cout << "==> Detectado: " << nome << " (PID=" << pid << ")\n";

                    string caminhoExe;
                    if (GetProcessImagePath(pid, caminhoExe)) {
                        cout << "  Caminho do exe: " << caminhoExe << "\n";

                        // 1) tenta encerrar o processo
                        if (TerminateProcessByPID(pid)) {
                            cout << "  Processo encerrado com sucesso.\n";
                            // 2) tenta apagar o executável
                            DeleteExecutable(caminhoExe);
                        } else {
                            cout << "  Falha ao encerrar processo. Arquivo nao sera excluido.\n";
                        }

                    } else {
                        cout << "  Nao foi possivel obter o caminho do exe (permissoes?)\n";
                    }
                    cout << "-------------------------------------------\n";
                }
            }
        } while (Process32NextW(snapshot, &processo));
    }
    CloseHandle(snapshot);
}

int main() {
    cout << "Monitor iniciando (apagar .exe). Ctrl+C para parar.\n";
    while (true) {
        verificarEBloquear();
        Sleep(10000); // 10s
    }
    return 0;
}
