#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <sys/socket.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <lib.h>
#include <sockets.h>

#include "ManejoColas.h"

bool esis_desbloqueados;
bool pausado_consola;

void ejecutar_consola();

void console_pausar();
void console_continuar();
void console_bloquear(char* clave_recurso, int id);
void console_desbloquear(char* clave);
void console_listar(char* recurso);
void console_kill(int id);
void console_status(char* clave);
void console_deadlock();
t_recurso* recurso_bloqueando_esi(t_esi* esi);
bool esi_bloqueado(t_esi* esi);

#endif /* CONSOLA_H_ */
