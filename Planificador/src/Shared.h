#ifndef SHARED_H_
#define SHARED_H_

#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/queue.h>

#define FIFO  1
#define SJF_SIN_DESALOJO  2
#define SJF_CON_DESALOJO  3
#define HRRN 4

t_log* logger;
t_log* logger_consola;
t_config* config;
t_queue * cola_listos;
t_queue * cola_bloqueados;
t_queue * cola_ejecutando;
t_queue * cola_terminados;
t_queue * cola_nuevos;
t_list * recursos;
bool planificador_bloqueado;
bool esis_desbloqueados;
bool pausado_consola;
int PUERTO_COORDINADOR;
char* IP_COORDINADOR;
char* ALGORITMO;
int ESTIMACION;
char** CLAVES_BLOQUEADAS;
int ALPHA;
int PUERTO_PLANIFICADOR;


pthread_mutex_t mutex_console;

pthread_t t_consola;

void liberar_cola(t_queue* cola);
void liberar_lista(t_list* lista);
void liberar_lista_recursos();
void planificador_status(char* clave);

typedef struct esi {
	int id;
	int rafaga;
	float estimacion;
	float rafaga_restante;
	int tiempo_esperando;
}t_esi;

typedef struct recurso {
	char* clave;
	t_queue* esis;
}t_recurso;


#endif /* SHARED_H_ */



//DOCUMENTACION//

/*
	1) Una vez levantadas las configuraciones necesarias se esperara la conexion de esis
	2) Cuando sea necesario que un esi ejecute se usara la funcion planificar(algoritmo), la misma despendiendo del algoritmo
		dejara en la cola de ejecutando al esi a ejecutar y devolvera el esi a ejecutar
	3) El esi a ejecutar sera enviado a la funcion enviar_mensaje_a_esi(esi) y se le dira que ejecute al esi en cuestion
	4) Se quedara esperando a la respuesta por parte del coordinador
	5) Se recibe una respuesta del coordinador
		5.1) El coordinador responde con OK
			5.1.1) El coordinador enviar un mensaje diciendo que el esi quiso hacer un get sobre un recurso
				1) Se ejecutara la function get_recurso(recurso) que se encargara de ver si el recurso es asignable o no
				2.1) Si es asignable y existe en el sistema, agregara al esi a la cola de esis del recurso, indicando que esta
					usando dicho recurso. get_recurso(recurso) devuelve true
				2.2) Si es asignable y no existe, creara el recurso y asignara el esi a la cola de esis del recurso.
					get_recurso(recurso) devuelve true
				2.3) Si no es asignable el esi sera movido de la cola de ejecutando a la de bloqueados, se agregara el esi a la
					cola de esis del recurso. get_recurso(recurso) devuelve false
			5.1.2) El coordinador enviar un mensaje diciendo que el esi quiso hacer un set sobre un recurso
				1) Se ejecutara la funcion set_recurso(recurso) que solo devuelve true
			5.1.3) El coordinador enviar un mensaje diciendo que el esi quiso hacer un store sobre un recurso
				1) Se ejecutara la funcion store_recurso(recurso) que se encargara de sacar el esi que esta ejecutando de la cola
					de esis del recurso, para liberar dicho recurso. El recurso quedara tomado por el siguiente ese en la cola de
					esis del recurso. store_recurso(recurso) devuelve true
		6.1) Se ejecutara la function notificar_planificador_resultado_operacion(resultado)
			6.1.1) El resultado el false significando que no se realizo la operacion
				1) Se notifica al coordinador a travez de la funcion enviar_mensaje_a_coordinador()
				1) Se ejecuta la funcion planificar(algoritmo)
			6.1.2) El resultado el true
				1) Se notifica al coordinador a travez de la funcion enviar_mensaje_a_coordinador()
				2.1) El algoritmo es FIFO
					1) Se ejecuta la funcion enviar_mensaje_a_asi(esi) con el esi ejecutando
				2.1) El algoritmo es SJF sin desalojo
					1) se suma 1 a la rafaga del esi y se ejecuta la funcion enviar_mensaje_a_asi(esi) con el esi ejecutando
				2.1) El algoritmo es SJF con desalojo
					1) se suma 1 a la rafaga del esi y se replanifica
					2) Se ejecuta la funcion enviar_mensaje_a_asi(esi) con el resultado de la replanificacion
 */









