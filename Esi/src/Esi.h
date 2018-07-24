/*
 * coordinador.h
 *
 *  Created on: 21 abr. 2018
 *      Author: utnso
 */

#ifndef ESI_H
#define ESI_H

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <sockets.h>
#include <commons/config.h>
#include <lib.h>
#include <protocolos.h>
#include <commons/collections/queue.h>


#define programa "ESI"

char* ip_planificador;
int puerto_planificador;
char* ip_coordinador;
int puerto_coordinador;
char* pathScript;

void _list_elements(char * sentencia);
void obtenerSentenciasDeScript(char * path);
int parsearSentencia(char *sentencia, char** clave, char** valor);
//-------config------------

char* ruta_config;// =  "/home/utnso/Escritorio/tp-2018-1c-La-Cuarta-Es-La-Vencida/Esi/resources/config.cfg";
t_config* config;
void parsear_config();

//-------logger------------

char* ruta_log = "logger.log";
t_log_level level = LOG_LEVEL_INFO;
t_log* logger;
void levantar_logger();

//---------SOCKET COORDINADOR ---

int s_coordinador;
int s_planificador;

void exit_gracefully(int return_nr);
void conectarse_coord();
void conectarse_planif();
void procesarScript();
void liberarRecursos();
void iniciarEsi(char* ruta);

#endif /* ESI_H */
