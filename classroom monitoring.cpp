// main.cpp
#include <iostream> // Para entrada e saída no console
#include <filesystem> // Para manipulação de arquivos e pastas
#include <thread>// Para criar threads (execução paralela)
#include <shlobj.h>  // Para usar SHEmptyRecycleBinW (função da Shell)
#include "bloqueio.h" // Cabeçalho personalizado, contém "iniciarMonitoramento"
#include <minwindef.h>
#include <libloaderapi.h>
#include <winerror.h>

#pragma comment(lib, "Shell32.lib") // se estiver usando MSVC, garante link com Shell32

using namespace std;
namespace fs = std::filesystem;

void limparPasta(const string& caminho) {
    try {
        if (!fs::exists(caminho)) {
            cout << "Caminho nao existe: " << caminho << endl;
            return;
        }
        if (!fs::is_directory(caminho)) {
            cout << "Nao e uma pasta: " << caminho << endl;
            return;
        }

        for (const auto& entry : fs::directory_iterator(caminho)) {
            // remove_all para apagar recursivamente arquivos/pastas
            std::error_code ec;
            fs::remove_all(entry.path(), ec);
            if (ec) {
                cout << "Falha ao remover " << entry.path().string() << ": " << ec.message() << endl;
            }
        }
        cout << "Pasta limpa: " << caminho << endl;
    }
    catch (const fs::filesystem_error& e) {
        cout << "Erro ao limpar " << caminho << ": " << e.what() << endl;
    }
}
#pragma comment(lib, "Shell32.lib")


int main() {
    cout << "Iniciando agente de limpeza escolar..." << endl;

    // Inicia o monitoramento em outra thread; podemos escolher:
    // - deixar a thread em detach() (monitoramento roda enquanto processo existir)
    // - ou join() mais tarde (bloquear main e manter agente vivo)
    // Aqui vamos detach() para o exemplo (mas lembre-se: se main terminar, o processo finaliza e o monitor para).
    thread monitorThread(iniciarMonitoramento);
    monitorThread.detach();

    // --- Limpeza segura: testar antes de apagar --- //
    // Recomendo trocar pelos caminhos de teste antes de rodar em pastas reais.
    limparPasta("C:\\Users\\rhays\\Music");
    limparPasta("C:\\Windows\\Temp");

    // Esvaziar a lixeira (sem confirmação)
    HMODULE shell = GetModuleHandle(L"Shell32");
    // SHEmptyRecycleBinW está em Shell32.lib; chamamos diretamente:
    HRESULT hr = SHEmptyRecycleBinW(NULL, NULL, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI);
    if (SUCCEEDED(hr)) {
        cout << "Lixeira esvaziada." << endl;
    }
    else {
        cout << "Falha ao esvaziar a lixeira. HR = " << hr << endl;
    }

    cout << "Limpeza concluida com sucesso!" << endl;

    // Mantem o programa vivo para o monitoramento (evita exit imediatamente)
    cout << "Pressione Enter para encerrar o agente..." << endl;
    cin.get();

    return 0;
}
