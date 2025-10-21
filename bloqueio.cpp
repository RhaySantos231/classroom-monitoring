#include <iostream>      // Biblioteca padrão para entrada e saída (cout, cin)
#include <windows.h>     // Biblioteca principal do Windows (para funções do sistema)
#include <tlhelp32.h>    // Necessária para manipular e listar processos
#include <vector>        // Para armazenar a lista de programas bloqueados
#include <string>        // Para trabalhar com strings (nomes dos processos)

using namespace std;

// 🚫 Lista de programas proibidos
vector<string> programasBloqueados = {
    "notepad.exe",     // Exemplo: Bloco de notas
    "roblox.exe",      // Roblox
    "discord.exe",     // Discord
    "minecraft.exe"    // Minecraft
};

// 🔍 Função que verifica todos os processos em execução e encerra os que estão bloqueados
void verificarEBloquear()
{
    // Cria um snapshot (uma "foto") de todos os processos rodando no sistema
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        cout << "❌ Erro ao criar snapshot dos processos." << endl;
        return;
    }

    PROCESSENTRY32 processo;       // Estrutura que armazena informações sobre cada processo
    processo.dwSize = sizeof(PROCESSENTRY32);

    // Inicia a listagem dos processos
    if (Process32First(snapshot, &processo))
    {
        do
        {
            // Converte o nome do processo de WCHAR para string normal
            wstring nomeW(processo.szExeFile);
            string nome(nomeW.begin(), nomeW.end());

            // Percorre a lista de bloqueados
            for (string bloqueado : programasBloqueados)
            {
                // Compara ignorando maiúsculas/minúsculas
                if (_stricmp(nome.c_str(), bloqueado.c_str()) == 0)
                {
                    // Tenta abrir o processo com permissão para encerrá-lo
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processo.th32ProcessID);
                    if (hProcess)
                    {
                        TerminateProcess(hProcess, 0); // Encerra o processo
                        CloseHandle(hProcess);

                        cout << " Programa bloqueado e encerrado: " << nome << endl;
                    }
                }
            }

        } while (Process32Next(snapshot, &processo)); // Continua até o fim da lista
    }

    // Fecha o snapshot para liberar memória
    CloseHandle(snapshot);
}

void iniciarMonitoramento()
{
    cout << " Iniciando o sistema de bloqueio de programas..." << endl;
    cout << "Monitorando processos proibidos a cada 5 segundos." << endl << endl;

    while (true)
    {
        verificarEBloquear(); // Chama a função que verifica os processos
        Sleep(10000);          // Espera 5 segundos antes de verificar novamente
    }

 

}
