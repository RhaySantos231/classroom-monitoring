// bloqueio.h
#pragma once

#include <string>
#include <vector>

// Inicia o monitoramento contínuo de processos conforme a lista interna.
// Esta função bloqueia (encerra) processos listados e fica em loop infinito.
// Recomenda-se chamar isso em uma std::thread (ex.: std::thread(iniciarMonitoramento).detach()).
void iniciarMonitoramento();

// Substitui a lista de programas bloqueados. Recebe nomes de executáveis (ex: "notepad.exe").
// Use esta função antes de iniciar o monitoramento para configurar os alvos.
void setProgramasBloqueados(const std::vector<std::string>& programas);

// Adiciona um programa à lista de bloqueados (não duplicará se já existir).
void adicionarProgramaBloqueado(const std::string& programa);

// Retorna a lista atual de programas bloqueados (cópia).
std::vector<std::string> obterProgramasBloqueados();
