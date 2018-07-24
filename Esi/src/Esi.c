/*
 ============================================================================
 Name        : Esi.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "Esi.h"


t_queue *sentencias;

int main(int argc, char *argv[]) {
	ruta_config = string_duplicate(argv[2]);
	iniciarEsi(argv[1]);
	conectarse_coord();
	conectarse_planif();
	obtenerSentenciasDeScript(pathScript);
	procesarScript();
	liberarRecursos();
}

void iniciarEsi(char* ruta){
	pathScript = string_duplicate(ruta);
	sentencias = queue_create();
	levantar_logger();
	parsear_config();
}

void liberarRecursos(){
	free(ip_planificador);
	free(ip_coordinador);
	free(pathScript);
	log_destroy(logger);
	int cantidad_elementos = queue_size(sentencias);
	int pos;
	for(pos = 0; pos < cantidad_elementos; pos++){
		queue_pop(sentencias);
	}
	queue_destroy(sentencias);
}

void procesarScript(){
	while(1){
		char *clave = string_new();
		char *valor = string_new();
		int protocolo_por_error;
		int prot_planif = recibirProtocolo(s_planificador);
		if(prot_planif == EJECUTAR){
			char * sentencia_parsear = string_duplicate(queue_peek(sentencias));
			int aux = parsearSentencia(sentencia_parsear, &clave, &valor);
			free(sentencia_parsear);
			switch(aux){
				case GET:
					enviarMensajeConProtocolo(s_coordinador, clave, GET_KEY);
					log_info(logger, "Se envio el GET");
					break;
				case SET:
					enviarMensajeConProtocolo(s_coordinador, clave, SET_KEY);
					enviarMensajeConProtocolo(s_coordinador, valor, SET_VALUE);
					log_info(logger, "Se envio el SET");

					break;
				case STORE:
					enviarMensajeConProtocolo(s_coordinador, clave, STORE_KEY);
					log_info(logger, "Se envio el STORE");

					break;
				default:
					log_info(logger, "PROTOCOLO DE PARSEO ERRONEO");
					break;
			}
		 }

		int prot_coord = recibirProtocolo(s_coordinador);
		switch(prot_coord){
			case OPER_OK:
				queue_pop(sentencias);
				if(queue_size(sentencias) != 0){
					enviarProtocolo(s_planificador, OK_OPER);
					log_info(logger, "Se envio al planificador que la sentencia se ejecuto correctamente");
				}else{
					enviarProtocolo(s_planificador, FINAL_OPER);
					log_info(logger, "Se envio al planificador que la sentencia se ejecuto correctamente y fue la ultima");
					protocolo_por_error = recibirProtocolo(s_planificador);
					if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
					}
				}
				break;
			case OPER_ERROR_SIZE_CLAVE:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "Tamano de clave fuera de rango");
				protocolo_por_error = recibirProtocolo(s_planificador);
				if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
				}
				break;
			case OPER_ERROR_CLAVE_NO_IDENTIFICADA:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "clave no identificada por el Planificador");
				protocolo_por_error = recibirProtocolo(s_planificador);
				if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
				}
				break;
			case OPER_ERROR_CLAVE_INACCESIBLE:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "clave inaccesible para ESI en ejecucion");
				break;
			case OPER_ERROR_INST:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "problema en instancia");
				protocolo_por_error = recibirProtocolo(s_planificador);
				if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
				}
				break;
			case OPER_ERROR_DESCONEXION_INST:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "instancia desconectada");
				protocolo_por_error = recibirProtocolo(s_planificador);
				if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
				}
				break;
			case OPER_ERROR:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "Ocurrio un error en el coordinador.");
				protocolo_por_error = recibirProtocolo(s_planificador);
				if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
				}
				break;
			case OPER_ERROR_CLAVE_INACCESIBLE_ABORTA:
				enviarProtocolo(s_planificador, prot_coord);
				log_error(logger, "La clave no esta reservada por este ESI.");
				protocolo_por_error = recibirProtocolo(s_planificador);
				if (protocolo_por_error == ABORTEN){
					log_info(logger,"Se aborta el ESI en ejecucion");
					exit_gracefully(0);
				}
				break;
			case ABORTEN:
				log_info(logger,"Se aborta el ESI en ejecucion");
				exit_gracefully(0);
				break;
			case -1:
				log_error(logger,"Se desconecto el Coordinador. Comenzando aborto.");
				free(clave);
				free(valor);
				exit_gracefully(0);
				/* no break */
			default:
				log_error(logger, "PROTOCOLO ERRONEO DEL COORDINADOR");
				break;
		}
		free(clave);
		free(valor);
	}
}

void exit_gracefully(int return_nr) {
	liberarRecursos();
	exit(return_nr);
}

void obtenerSentenciasDeScript(char * path){
	FILE * fp;
	char* line = NULL;
	ssize_t read;
	size_t len = 0;
	fp = fopen(path, "r");
	if (fp == NULL)
	   exit(EXIT_FAILURE);
//TODO modificado para liberar memoria, hace falta chequear
	while ((read = getline(&line, &len , fp)) != -1) {
	   queue_push(sentencias, line);
	   len = 0;
	}

	fclose(fp);
	if (line)
	  free(line);

	log_info(logger, "Sentencias enlistadas");

}

void _list_elements(char * sentencia) {
	   printf("Sentencia %s :\n", sentencia);
}

int parsearSentencia(char *sentencia, char** clave, char** valor){

	t_esi_operacion parsed = parse(sentencia);

	if(parsed.valido){
	   switch(parsed.keyword){
	     case GET:
	    	 string_append(clave, parsed.argumentos.GET.clave);
	    	 log_info(logger, "Sentencia parseada: GET\tclave: %s\n", parsed.argumentos.GET.clave);
	    	 destruir_operacion(parsed);

	       return GET;
	       break;
	     case SET:
	    	 string_append(clave, parsed.argumentos.SET.clave);
	    	 string_append(valor, parsed.argumentos.SET.valor);
	       log_info(logger, "Sentencia parseada: SET\tclave: %s\tvalor: %s\n", parsed.argumentos.SET.clave, parsed.argumentos.SET.valor);
	 	  destruir_operacion(parsed);

	       return SET;
	       break;
	     case STORE:
	    	 string_append(clave, parsed.argumentos.STORE.clave);
	       log_info(logger, "Sentencia parseada: STORE\tclave: %s\n", parsed.argumentos.STORE.clave);
	 	  destruir_operacion(parsed);

	       return STORE;
	       break;
	     default:
		  log_info(logger, "error de parseo");
	 	  destruir_operacion(parsed);

	       exit(EXIT_FAILURE);
	     }


	}
    exit(EXIT_FAILURE);

}

void levantar_logger() {
	logger = abrir_logger(ruta_log, programa, LOG_LEVEL_INFO);
	log_info(logger, "Logger iniciado");
}

void parsear_config() {
	log_info(logger, "Levantando configuracion");
	config = config_create(ruta_config);
	ip_planificador = string_duplicate(config_get_string_value(config, "IP_PLANIFICADOR"));
	puerto_planificador = config_get_int_value(config, "PUERTO_PLANIFICADOR" );
	ip_coordinador = string_duplicate(config_get_string_value(config, "IP_COORDINADOR" ));
	puerto_coordinador= config_get_int_value(config, "PUERTO_COORDINADOR" );
	log_info(logger, "Configuracion levantada %s", ip_planificador);
	config_destroy(config);
	return;
}

void conectarse_coord(){
	printf("Intento conectarme a Coordinador\n");
	int p_resp;
	s_coordinador = conectarAuth(puerto_coordinador,ip_coordinador, ESI, &p_resp);
	if(p_resp == OKCONN){
		printf("Conectado a Coordinador con socket %d \n",s_coordinador);
	}else
	{
		printf("error de coneccion \n");
	}
}

void conectarse_planif(){
		printf("Intento conectarme a Coordinador\n");
		int p_resp;
		s_planificador = conectarAuth(puerto_planificador,ip_planificador, ESI, &p_resp);
		if(p_resp == OKCONN){
			printf("Conectado a fs con socket %d \n",s_coordinador);
		}else
		{
			printf("error de coneccion \n");
		}
}
