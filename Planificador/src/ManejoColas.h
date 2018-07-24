#ifndef COLAS_H_
#define COLAS_H_


#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/string.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <lib.h>
#include <protocolos.h>
#include "Shared.h"


void queue_iterate(t_queue* self, void(*closure)(void*));
t_queue* queue_filter(t_queue* self, bool(*condition)(void*));
int queue_count_satisfying(t_queue* self, bool(*condition)(void*));
bool queue_any_satisfy(t_queue* self, bool(*condition)(void*));
int definir_algoritmo();


void mover_esi(t_queue* origen,int nombreColaOrigen, t_queue* destino, int nombreColaDestino);
void mover_esi_manual(t_queue** origen, int nombreColaOrigen, t_queue* destino, int nombreColaDestino, t_esi* esi);
void bloquear_esi_manual(int id, char* clave);
bool recurso_existe(char* clave);
bool esta_usando_recurso(t_esi* esi, char* clave);
bool asignar_recurso(t_esi* esi, char* clave);
void liberar_recurso(t_esi* esi, char* clave);
bool get_recurso(char* clave);
t_proto store_recurso(char* clave);
void terminar_esi();
void bloquear_esi(t_esi esi, t_recurso recurso);
void matar_esi_ejecutando();
t_proto set_recurso(char* clave);
t_esi* find_esi_by_id(int id, char* cola);

typedef enum{
	COLA_EJECUTANDO,
	COLA_BLOQUEADOS,
	COLA_LISTOS,
	COLA_TERMINADOS,
	COLA_NUEVOS
}t_prototipo;
#endif /* CONSOLA_H_ */
