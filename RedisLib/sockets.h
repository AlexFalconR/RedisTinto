/*
 * Al usar esto tenes que definir en tu .c
 * int tienePermiso(char* autentificacion){} y retornar 0 si no tiene y 1 si tiene permiso
 */
#ifndef REDISLIB_SOCKETS_H_
#define REDISLIB_SOCKETS_H_

#include "protocolos.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <commons/log.h>


///////////
//CLIENTE//
///////////
int conectar(int puerto,char* ip);
int conectarAuth(int puerto,char* ip, int AUTH,int* p_respuesta);

////////////
//SERVIDOR//
////////////
int crearServidor(int puerto);
int esperarConexion(int servidor);
int esperarConexionAuth(int servidor, int *AUTH);
/////////////////////
//ENVIO DE MENSAJES//
/////////////////////
void enviarMensajeConProtocolo(int conexion, char* mensaje, int protocolo);
void enviarProtocolo(int socket, int protocolo);
int sendIntTo(int socket, int number);
void enviarMensaje(int conexion, char* mensaje);

/////////////////////////
//RECEPCION DE MENSAJES//
/////////////////////////
char* esperarMensaje(int conexion);
int recibirProtocolo(int conexion);



//////////////
//AUXILIARES//
//////////////
int autentificar(int conexion, char* autor);
int esperarConfirmacion(int conexion);
int aceptar(int servidor);
char* header(int numero);




#endif
