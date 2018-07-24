
#include "Consola.h"
#include "Shared.h"


void console_pausar(){
	if(!pausado_consola && !planificador_bloqueado){
		pthread_mutex_lock(&mutex_console);
		pausado_consola = true;
	}
}
void console_continuar(){
	if(pausado_consola){
		pthread_mutex_unlock(&mutex_console);
		pausado_consola = false;
	}
	if(!planificador_bloqueado && esis_desbloqueados && !pausado_consola){
		esis_desbloqueados = false;
		enviar_mensaje_a_esi(planificar(definir_algoritmo()));
	}
}

void pausar(){
	if(!pausado_consola && !planificador_bloqueado){
		pthread_mutex_lock(&mutex_console);
	}
}
void continuar(){
	if(!pausado_consola && !planificador_bloqueado){
		pthread_mutex_unlock(&mutex_console);
	}
}

void console_bloquear(char* clave_recurso, int id){
	pausar();
	char* nombre_cola = string_new();
	t_esi* esi = find_esi_by_id(id, nombre_cola);
	if(!esi_bloqueado(esi)){
		if(string_equals_ignore_case(nombre_cola, "cola_listos") || string_equals_ignore_case(nombre_cola, "cola_bloqueados")){
			log_info(logger, "Se bloquea el esi: %d, esperarando el recurso: %s\n", id, clave_recurso);
			asignar_recurso(esi, clave_recurso);
		}else{
			log_info(logger, "Se intento bloquear un esi que no esta en la cola de listos ni bloqueados\n");
		}
	}else{
		log_info(logger, "El esi ya esta siendo bloqueado\n");
	}

	continuar();
}

void console_desbloquear(char* clave){
	pausar();

	t_esi* primer_esi_bloqueado;
	t_list* recursos_aux = list_create();
	void copiar(t_recurso* recurso){
		list_add(recursos_aux, recurso);
	}
	list_iterate(recursos, (void *)copiar);
	bool coincide_clave(t_recurso* recurso){
			 if(strcmp(recurso->clave, clave) == 0){
				 if(queue_size(recurso->esis) < 2){
					 return NULL;
					 log_info(logger, "No hay esis bloqueados por dicho recurso");
				 }
				 queue_pop(recurso->esis);
				 primer_esi_bloqueado = queue_peek(recurso->esis);
				 return true;
			 }else{
				 return false;
			 };
			 return NULL;
		}

	if(list_any_satisfy(recursos_aux,(void*) coincide_clave)){
		log_info(logger, "Se desbloquea el esi: %d", primer_esi_bloqueado->id);
			mover_esi_manual(&cola_bloqueados, COLA_BLOQUEADOS, cola_listos, COLA_LISTOS, primer_esi_bloqueado);
			esis_desbloqueados = true;
	}else{
		log_info(logger, "La clave no existe. Intente nuevamente");
	}


	continuar();
}

void console_listar(char* clave_recurso){
	pausar();
	log_info(logger, "Esis bloqueados por el recurso: %s\n", clave_recurso);

	void coincide_clave(t_recurso* recurso){
				 if(strcmp(recurso->clave, clave_recurso) == 0){
					 void listar_esi(t_esi* esi){
						 log_info(logger, "Esi: %d\n", esi->id);
					 }
					 queue_iterate(recurso->esis,(void*) listar_esi);
				 };
			}
		list_iterate(recursos, (void *)coincide_clave);
	continuar();
}

void console_kill(int id){
	pausar();
	log_info(logger, "Se mata al esi: %d\n", id);
	matar_esi_manual(id);
	continuar();
}

void console_status(char* clave_recurso){
	pausar();
	log_info(logger, "Status de recurso: %s\n", clave_recurso );
	continuar();
	//Proporciona informacion de una instancia
}

bool esi_bloqueado(t_esi* esi){
	bool _esi(t_esi* esi_bloqueado){
		return (esi->id == esi_bloqueado->id);
	}
	return queue_any_satisfy(cola_bloqueados,(void *) _esi);
}

t_queue* esis_bloqueados(t_recurso* recurso){
	t_queue* esis = queue_create();
	t_esi* primer_esi = queue_peek(recurso->esis);
	void agregar_esi(t_esi* esi){
		if(esi->id != primer_esi->id){
			queue_push(esis, esi);
		}
	}
	queue_iterate(recurso->esis, (void *)agregar_esi);
	return esis;
}

t_recurso* recurso_bloqueando_esi(t_esi* esi){
	t_recurso* recurso_bloqueando;
	void _recurso(t_recurso* recurso){
		void _esi(t_esi* esi_encontrado){
			if(esi_encontrado->id == esi->id){
				recurso_bloqueando = recurso;
			}
		}
		queue_iterate(esis_bloqueados(recurso), (void*) _esi);
	}
	list_iterate(recursos,(void *) _recurso);

	return recurso_bloqueando;
}

void console_deadlock(){
	pausar();
	log_info(logger, "Esis en deadlock: \n");
		if(queue_size(cola_bloqueados) >= 2){
			t_esi* primer_esi;
			t_list* esis_involucrados;
			void detectar_deadlock(t_esi* esi){
				t_recurso* recurso = recurso_bloqueando_esi(esi);
				if(recurso != NULL){
					if(queue_size(recurso->esis) > 0){
						t_esi* esi_usando_recurso = queue_peek(recurso->esis);
						if(esi_usando_recurso->id == primer_esi->id){
							log_info(logger, "DEADLOCK :O \n");
							void loggear_esis(t_esi* esi){
								log_info(logger, "ESI %d \n", esi->id);
							}
							list_iterate(esis_involucrados, (void *)loggear_esis);
						}else{
							list_add(esis_involucrados, esi_usando_recurso);
							detectar_deadlock(queue_peek(recurso->esis));
						}
					}
				}
			}
			void _recurso(t_recurso* recurso){
				esis_involucrados = list_create();
				if(! queue_is_empty(recurso->esis)){
					primer_esi = queue_peek(recurso->esis);
					list_add(esis_involucrados, primer_esi);
					if(primer_esi->id != -1){
						detectar_deadlock(primer_esi);
					}
				}
			}
			list_iterate(recursos, (void *)_recurso);

		}else{
			log_info(logger, "Cola de bloqueados tiene menos de 2 esis, no es posible un deadlock \n");
		}
	continuar();
}
void console_loggear_esis(){
	pausar();
	loggear_colas();
	continuar();
}

void console_loggear_recursos(){
	pausar();
	loggear_recursos();
	continuar();
}

int cantidadElementos(char ** array) {
	size_t count = 0;
	while (array[count] != NULL)
		count++;
	return count;
}

void liberar_array_strings(char** array){
	int cantidad = cantidadElementos(array);
	int pos;
	for (pos = 0; pos < cantidad; pos++){
		free(array[pos]);
	}
}

void ejecutar_consola(){
		esis_desbloqueados = false;
		pausado_consola = false;
	  char * linea;
	  pthread_mutex_unlock(&mutex_console);
	  while(1) {
	    linea = readline(">");

//	    ABORTO CONSOLA
	    if (!linea || string_starts_with(linea, "exit")) {
	      break;
	    }
//	    RECONOZCO COMANDOS
	    if(string_equals_ignore_case(linea, "pausar")){
	    	// > pausar
	    	console_pausar();
	    	log_info(logger, "Ejecucion pausada.\n");
	    }else if(string_starts_with(linea, "continuar")){
	    	// > continuar
	    	console_continuar();
	    	log_info(logger, "Continua la ejecucion\n");
	    }else if(string_starts_with(linea,"bloquear ")){
	    	// > bloquear <clave> <valor>
	    	char** array_argumentos = string_split(linea, " ");
	    	char* clave_recurso = string_duplicate(array_argumentos[1]);
	    	char* id = string_duplicate(array_argumentos[2]);
	    	if(cantidadElementos(array_argumentos) == 3){
	    		console_bloquear(clave_recurso, strtol(id,NULL,10));
	    	}else{
	    		log_info(logger, "Cantidad erronea de argumentos\n");
	    	}
	    	//string_iterate_lines(array_argumentos, (void*) free);
	    	//free(clave_recurso); TODO: Ver porq esta linea rompe
	    	free(id);
	    	liberar_array_strings(array_argumentos);
	    	free(array_argumentos);
	    }else if(string_starts_with(linea,"desbloquear ")){
	    	// > desbloquear <id>
	    	char* clave = string_substring(linea, 12, strlen(linea));
	    	if(!string_is_empty(clave)){
	    		console_desbloquear(clave);
	    	}else{
	    		log_info(logger, "Cantidad erronea de argumentos\n");
	    	}
	    	free(clave);

	    }else if(string_starts_with(linea,"listar ")){
	    	// > listar <recurso>
	    	char* recurso = string_substring(linea, 7, strlen(linea));
	    	if(!string_is_empty(recurso)){
	    		console_listar(recurso);
	       	}else{
	       		log_info(logger, "Cantidad erronea de argumentos\n");
	    	}
	    	free(recurso);

	    }else if(string_starts_with(linea,"kill ")){
	    	// > kill <id>
	    	char* id = string_substring(linea, 5, strlen(linea));
	    	if(!string_is_empty(id)){
	    		console_kill(strtol(id,NULL,10));
	    	}else{
	    		log_info(logger, "Cantidad erronea de argumentos\n");
	    	}
	    	free(id);

	    }else if(string_starts_with(linea,"status ")){
	    	// > status <clave>
	    	char* clave = string_substring(linea, 7, strlen(linea));
	    	if(!string_is_empty(clave)){
	    		console_status(clave);
	    	}else{
	    		log_info(logger, "Cantidad erronea de argumentos\n");
	    	}
	    	free (clave);

	    }else if(string_starts_with(linea,"deadlock")){
	    	// > deadlock
	    	console_deadlock();
	    }else if(string_starts_with(linea,"loggearEsis")){
	    	console_loggear_esis();
	    }else if(string_starts_with(linea,"loggearRecursos")){
	    	// > deadlock
	    	console_loggear_recursos();
	    }else {
	    	log_info(logger, "El comando ingresado no existe. Intente nuevamente.\n");
	    }

	    free(linea);
	  }

}

