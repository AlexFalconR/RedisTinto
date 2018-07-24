
#include "ManejoColas.h"

t_esi* find_esi_by_id(int id, char* cola){

	t_esi* esi;

	bool coincide_id(t_esi* esi_encontrado) {
			if(esi_encontrado->id == id){
				esi = esi_encontrado;
				return true;
			}else{
				return false;
			};
			return NULL;
		}
	if (queue_any_satisfy(cola_listos, (void *) coincide_id)){
		string_append(&cola, "cola_listos");
	}else if(queue_any_satisfy(cola_ejecutando, (void *) coincide_id)){
		string_append(&cola, "cola_ejecutando");
	}else if(queue_any_satisfy(cola_bloqueados, (void *) coincide_id)){
		string_append(&cola, "cola_bloqueados");
	}

	return esi;
}

void re_estimar_esi_ejecutando(t_esi* esi){
	int algoritmo = definir_algoritmo();
	if(algoritmo == SJF_CON_DESALOJO){
		if(queue_size(cola_ejecutando) > 0){
			t_esi * esi_ejecutando = queue_peek(cola_ejecutando);
			if(esi_ejecutando->id != esi->id){
				t_esi * esi_ejecutando = queue_peek(cola_ejecutando);
				esi_ejecutando->rafaga_restante = esi_ejecutando->estimacion - esi_ejecutando->rafaga;
			}
		}
	}
}

void estimar_esi(t_esi* esi, int nombreColaOrigen, int nombreColaDestino){
	if(nombreColaOrigen == COLA_BLOQUEADOS && nombreColaDestino == COLA_LISTOS){
		float alpha = ALPHA;
		alpha= alpha/100;
		esi->estimacion = (alpha * esi->rafaga) + ((1-alpha) * esi->estimacion);
		esi->rafaga_restante = esi->estimacion;
		esi->rafaga = 0;
		re_estimar_esi_ejecutando(esi);
	} else if (nombreColaOrigen == COLA_NUEVOS && nombreColaDestino == COLA_LISTOS){
		esi->estimacion = ESTIMACION;
		esi->rafaga = 0;
		esi->rafaga_restante = ESTIMACION;
		esi->tiempo_esperando = 0;
		re_estimar_esi_ejecutando(esi);
	}

}

void mover_esi(t_queue* origen, int nombreColaOrigen, t_queue* destino, int nombreColaDestino){ //Testeada

	t_esi* esi = queue_pop(origen);
	estimar_esi(esi, nombreColaOrigen, nombreColaDestino);
	queue_push(destino, esi);

}

void mover_esi_manual(t_queue** origen, int nombreColaOrigen, t_queue* destino, int nombreColaDestino, t_esi* esi){

	estimar_esi(esi, nombreColaOrigen, nombreColaDestino);

	t_queue* cola_aux = queue_create();
	void coincide_id(t_esi* esi_encontrado) {
		/*t_esi* esi_removido = queue_pop(origen);
		if(esi_encontrado->id != esi->id){
			queue_push(origen, esi_removido);
		}else{
			queue_push(destino, esi_removido);
		}*/
		if (esi_encontrado->id == esi->id){
			queue_push(destino, esi_encontrado);
		}else{
			queue_push(cola_aux, esi_encontrado);
		};
	}
	queue_iterate(*origen, (void*) coincide_id);
//	liberar_cola(origen);
	*origen = cola_aux; //TODO: Liberar los recursos

}

bool recurso_existe(char* clave){ //Testeada
	bool coincide_clave(t_recurso* recurso){
		return (string_equals_ignore_case(recurso->clave, clave));
	}
	return list_any_satisfy(recursos, (void*) coincide_clave);
}

bool esta_esperando_recurso(t_esi* esi, char* clave){ //Testeado

	bool coincide_id(t_esi* esi_encontrado){
		if(esi_encontrado->id == esi->id){
			return true;
		} else{
			return false;
		}
	}

	bool buscar_clave(t_recurso* recurso) {
		 if(string_equals_ignore_case(recurso->clave,clave)){
			return queue_any_satisfy(recurso->esis, (void*) coincide_id);
		 }else{
			 return false;
		 }
	}

	return list_any_satisfy(recursos, (void*) buscar_clave);

}

bool esta_usando_recurso(t_esi* esi, char* clave){ //Testeado

	bool buscar_clave(t_recurso* recurso) {
		 if(string_equals_ignore_case(recurso->clave,clave)){
			 if(queue_is_empty(recurso->esis)){
				 return false;
			 }
			 t_esi* esi_usando_recurso = queue_peek(recurso->esis);
			return (esi_usando_recurso->id == esi->id);
		 }else{
			 return false;
		 }
	}

	return list_any_satisfy(recursos, (void*) buscar_clave);

}


bool recurso_desocupado(char* clave){
	bool buscar_clave(t_recurso* recurso) {
			 if(string_equals_ignore_case(recurso->clave,clave)){
				return queue_is_empty(recurso->esis);
			 }else{
				 return false;
			 }
		}

		return list_any_satisfy(recursos, (void*) buscar_clave);
}

bool asignar_recurso(t_esi* esi, char* clave){ //Testeado
	if(recurso_existe(clave)){
		if(esta_usando_recurso(esi, clave)){
			return true;
		}


		void agregar_esi(t_recurso* recurso) {

			if(strcmp(recurso->clave,clave) == 0){
				queue_push(recurso->esis, (void*) esi);
			}
		}

		if(recurso_desocupado(clave)){
			list_iterate(recursos, (void*) agregar_esi);
			return true;
		}else{
			list_iterate(recursos, (void*) agregar_esi);
			mover_esi(cola_ejecutando, COLA_EJECUTANDO, cola_bloqueados, COLA_BLOQUEADOS);
			return false;
		}

	}else{
		//TODO: Modificado por Al3x
		t_recurso* recurso = malloc(sizeof(t_recurso));
		recurso->clave = string_duplicate(clave);
		recurso->esis = queue_create();
		queue_push(recurso->esis, (void*) esi);
		list_add(recursos, (void*) recurso);
		return true;
	}

}

t_list* recursos_requeridos_por_esi(t_esi* esi){
	t_list* lista_claves = list_create();
	t_recurso* recurso;
	void _cola(t_esi* esi_encontrado){
		t_esi* primer_esi = queue_peek(recurso->esis);
			if(esi->id == esi_encontrado->id && esi->id != primer_esi->id){
				list_add(lista_claves, recurso->clave);
			}
		}
	void _recurso(t_recurso* recurso_encontrado){
		recurso = recurso_encontrado;
		queue_iterate(recurso_encontrado->esis, (void*) _cola);
	}
	list_iterate(recursos, (void*) _recurso);
	return lista_claves;
}

void liberar_recurso(t_esi* esi, char* clave){ //Testeado
	void buscar_clave(t_recurso* recurso) {
		if(esta_usando_recurso(esi, recurso->clave) && clave == NULL){
			queue_pop(recurso->esis);// Saco el esi que estaba usando el recurso y lo libero
			if(!queue_is_empty(recurso->esis)){
				mover_esi_manual(&cola_bloqueados, COLA_BLOQUEADOS, cola_listos, COLA_LISTOS, queue_peek(recurso->esis));
			}
		}else if(esta_usando_recurso(esi, recurso->clave) && string_equals_ignore_case(recurso->clave, clave)){
			queue_pop(recurso->esis);// Saco el esi que estaba usando el recurso y lo libero
			if(!queue_is_empty(recurso->esis)){
					mover_esi_manual(&cola_bloqueados, COLA_BLOQUEADOS, cola_listos, COLA_LISTOS, queue_peek(recurso->esis));
			}
		}
	}
	list_iterate(recursos, (void*) buscar_clave);
}

void sacar_esi_de_recursos(t_esi* esi){

	bool coincide_id(t_esi* esi_encontrado) {
		return (esi_encontrado->id != esi->id);
	}
	void ver_esis(t_recurso* recurso){
		recurso->esis = queue_filter(recurso->esis, (void*) coincide_id);
	}
	list_iterate(recursos, (void *) ver_esis);
}

bool get_recurso(char* clave){ //Testeado

	if(queue_size(cola_ejecutando) != 0){
		t_esi* esi_ejecutando = queue_peek(cola_ejecutando);
		esi_ejecutando->rafaga++;
		esi_ejecutando->tiempo_esperando = 0;
		void sumar_tiempo_espera(t_esi* esi){
			if(esi->id != esi_ejecutando->id){
				esi->tiempo_esperando++;
			}
		}
		queue_iterate(cola_listos,(void *) sumar_tiempo_espera);
		return asignar_recurso(esi_ejecutando, clave);
	}else{
		log_info(logger, "GET ABORTADO: no hay esi ejecutando");
		return false;
	}
}

t_proto store_recurso(char* clave){
	if(queue_size(cola_ejecutando) != 0){
		t_esi* esi_ejecutando = queue_peek(cola_ejecutando);
		esi_ejecutando->rafaga ++;
		esi_ejecutando->tiempo_esperando = 0;
		void sumar_tiempo_espera(t_esi* esi){
			if(esi->id != esi_ejecutando->id){
				esi->tiempo_esperando++;
			}
		}
		queue_iterate(cola_listos,(void *) sumar_tiempo_espera);
		if(!recurso_existe(clave)){
			return OPER_ERROR_CLAVE_NO_IDENTIFICADA;
		}
		else if(!(esta_usando_recurso(esi_ejecutando,clave))) {
			return OPER_ERROR_CLAVE_INACCESIBLE_ABORTA; //CLAVE QUE EXISTE PARA EL PLANIFICADOR PERO NO PARA EL COORDINADOR
		}else
			liberar_recurso(esi_ejecutando, clave);
			return CONTINUAR_OPER;
	}else{
		return OPER_ERROR_DESCONEXION_ESI;
	}
}

t_proto set_recurso(char* clave){
	//TODO: MODIFICADO POR ALEX
	if(queue_size(cola_ejecutando) != 0){
		t_esi* esi_ejecutando = queue_peek(cola_ejecutando);
		esi_ejecutando->rafaga ++;

		esi_ejecutando->tiempo_esperando = 0;
		void sumar_tiempo_espera(t_esi* esi){
			if(esi->id != esi_ejecutando->id){
				esi->tiempo_esperando++;
			}
		}
		queue_iterate(cola_listos,(void *) sumar_tiempo_espera);

		if(!(esta_usando_recurso(esi_ejecutando,clave))) {
			return OPER_ERROR_CLAVE_INACCESIBLE; //CLAVE QUE EXISTE PARA EL PLANIFICADOR PERO NO PARA EL COORDINADOR
		}else
			return CONTINUAR_OPER;
		}else{
			return OPER_ERROR_DESCONEXION_ESI;
		}
	log_info(logger, "No existen esis en la cola de ejecutando");
	return -1;

}

void terminar_esi(){ //Testeada
	mover_esi(cola_ejecutando, COLA_EJECUTANDO, cola_terminados, COLA_TERMINADOS);
}

void bloquear_esi_ejecutando(t_esi esi){ // TEsteada
	mover_esi(cola_ejecutando, COLA_EJECUTANDO, cola_bloqueados, COLA_BLOQUEADOS);
	if(queue_size(cola_ejecutando) == 0){

	}
}

void bloquear_esi_manual(int id, char* clave){
	char* cola_donde_esta_el_esi = string_new();
	t_esi* esi_encontrado = find_esi_by_id(id, cola_donde_esta_el_esi);
	if(string_equals_ignore_case(cola_donde_esta_el_esi, "cola_listos")){
		mover_esi(cola_listos, COLA_LISTOS, cola_bloqueados, COLA_BLOQUEADOS);
		asignar_recurso(esi_encontrado, clave);
	}else if(string_equals_ignore_case(cola_donde_esta_el_esi, "cola_ejecutando")){
		mover_esi(cola_ejecutando, COLA_EJECUTANDO, cola_bloqueados, COLA_BLOQUEADOS);
		asignar_recurso(esi_encontrado, clave);
	}else{
		log_info(logger, "Se intento bloquear un esi que no estaba en cola de listos ni ejecutando");
	}
	//free(cola_donde_esta_el_esi);
}

//TODO: Metodo modificado por Alex
void matar_esi_ejecutando(int socket){
	if(queue_size(cola_ejecutando) != 0 ){
		t_esi* esi_ejecutando = queue_peek(cola_ejecutando);
		terminar_esi();
		enviarProtocolo(socket, ABORTEN);
		liberar_recurso(esi_ejecutando, NULL);
	}else{
		log_info(logger, "Se intento matar al esi ejecutando, pero cola_ejecutando esta vacia");
	}
}

void matar_esi_manual( int id){
	char* cola_donde_esta_el_esi = string_new();
	t_esi* esi_encontrado = find_esi_by_id(id, cola_donde_esta_el_esi);
	if(string_equals_ignore_case(cola_donde_esta_el_esi, "cola_listos")){
			mover_esi(cola_listos, COLA_LISTOS, cola_terminados, COLA_TERMINADOS);
		}else if(string_equals_ignore_case(cola_donde_esta_el_esi, "cola_ejecutando")){
			mover_esi(cola_ejecutando, COLA_EJECUTANDO, cola_terminados, COLA_TERMINADOS);
		}else if(string_equals_ignore_case(cola_donde_esta_el_esi, "cola_bloqueados")){
			mover_esi(cola_bloqueados, COLA_BLOQUEADOS, cola_terminados, COLA_TERMINADOS);
		}

	sacar_esi_de_recursos(esi_encontrado);
}

void generar_colas_prueba(){
	t_esi* esi1 = malloc(sizeof(t_esi));
	t_esi* esi2 = malloc(sizeof(t_esi));
	t_esi* esi3 = malloc(sizeof(t_esi));

	esi1->id = 1;
	esi2->id = 2;
	esi3->id = 3;
	esi1->rafaga = 0;
	esi2->rafaga = 0;
	esi3->rafaga = 0;
	esi1->rafaga_restante = 0;
	esi2->rafaga_restante = 0;
	esi3->rafaga_restante = 0;
	esi1->tiempo_esperando = 0;
	esi2->tiempo_esperando = 0;
	esi3->tiempo_esperando = 0;

	queue_push(cola_bloqueados, esi1);
	queue_push(cola_bloqueados, esi2);
	queue_push(cola_bloqueados, esi3);

	t_recurso* recurso1 = malloc(sizeof(t_esi));
	t_recurso* recurso2 = malloc(sizeof(t_esi));
	t_recurso* recurso3 = malloc(sizeof(t_esi));
	t_recurso* recurso4 = malloc(sizeof(t_esi));

	recurso1->clave = "1";
	recurso2->clave = "2";
	recurso3->clave = "3";
	recurso4->clave = "4";

	recurso1->esis = queue_create();
	queue_push(recurso1->esis, esi2);
	queue_push(recurso1->esis, esi1);
	recurso2->esis = queue_create();
	queue_push(recurso2->esis, esi3);
	queue_push(recurso2->esis, esi2);
	recurso3->esis = queue_create();
	queue_push(recurso3->esis, esi1);
	queue_push(recurso3->esis, esi3);
	recurso4->esis = queue_create();
	queue_push(recurso4->esis, esi3);
	queue_push(recurso4->esis, esi1);

	list_add(recursos, recurso1);
	list_add(recursos, recurso2);
	list_add(recursos, recurso3);
	list_add(recursos, recurso4);

	void asignar_estimacion_inicial(t_esi* esi){
		esi->estimacion = ESTIMACION;
	}
	queue_iterate(cola_listos, (void *) asignar_estimacion_inicial);
}


void loggear_colas(){

	void loggear_listos(t_esi* esi){
		log_info(logger, "Esi: %d en cola de listos, estimacion: %f, rafaga: %d\n, wait: %d, rr: %f", esi->id, esi->estimacion, esi->rafaga, esi->tiempo_esperando, (esi->estimacion + esi->tiempo_esperando)/esi->estimacion);
	}
	void loggear_bloqueados(t_esi* esi){
		log_info(logger, "Esi: %d en cola de bloqueados, estimacion: %f, rafaga: %d\n, wait: %d, rr: %f", esi->id, esi->estimacion, esi->rafaga, esi->tiempo_esperando, (esi->estimacion + esi->tiempo_esperando)/esi->estimacion);
	}
	void loggear_ejecutando(t_esi* esi){
		log_info(logger, "Esi: %d en cola de ejecutando, estimacion: %f, rafaga: %d\n, wait: %d, rr: %f", esi->id, esi->estimacion, esi->rafaga, esi->tiempo_esperando, (esi->estimacion + esi->tiempo_esperando)/esi->estimacion);
	}
	void loggear_terminados(t_esi* esi){
		log_info(logger, "Esi: %d en cola de terminados, estimacion: %f, rafaga: %d\n, wait: %d, rr: %f", esi->id, esi->estimacion, esi->rafaga, esi->tiempo_esperando, (esi->estimacion + esi->tiempo_esperando)/esi->estimacion);
	}

	queue_iterate(cola_listos, (void*) loggear_listos);
	queue_iterate(cola_bloqueados, (void*) loggear_bloqueados);
	queue_iterate(cola_ejecutando, (void*) loggear_ejecutando);
	queue_iterate(cola_terminados, (void*) loggear_terminados);
	log_info(logger, "----------------------------------------");
}


void loggear_recursos(){
	void loggear_cola(t_esi* esi){
			log_info(logger, "Esi: %d en cola, estimacion: %f, rafaga: %d\n", esi->id, esi->estimacion, esi->rafaga);
		}
	void loggear_listos(t_recurso* recurso){
		log_info(logger, "Recurso: %s: \n", recurso->clave);
		queue_iterate(recurso->esis, (void*) loggear_cola);
	}
	list_iterate(recursos, (void*) loggear_listos);
	log_info(logger, "----------------------------------------");
}

void loggear_recurso(char* clave){
	void loggear_cola(t_esi* esi){
			log_info(logger, "Esi: %d", esi->id);
		}
	void loggear_listos(t_recurso* recurso){
		log_info(logger, "Recurso: %s: \n", recurso->clave);
		if(string_equals_ignore_case(recurso->clave, clave)){
			queue_iterate(recurso->esis, (void*) loggear_cola);
		}
	}
	list_iterate(recursos, (void*) loggear_listos);
	log_info(logger, "----------------------------------------");
}

void debuggear_recursos(){
	void loggear_cola(t_esi* esi){

	}
	void loggear_listos(t_recurso* recurso){
		queue_iterate(recurso->esis, (void*) loggear_cola);
	}
	list_iterate(recursos, (void*) loggear_listos);
}

void queue_iterate(t_queue* self, void(*closure)(void*)) {
	t_link_element *element = self->elements->head;
	t_link_element *aux = NULL;
	while (element != NULL) {
		aux = element->next;
		closure(element->data);
		element = aux;
	}
}

t_queue* queue_filter(t_queue* self, bool(*condition)(void*)){
	t_queue* cola_aux = queue_create();

	void _add_if_apply(void* element) {
		if (condition(element)) {
			queue_push(cola_aux, element);
			//TODO: OJO CON ESTO, MODIFICO PERO CON MIEDO
//			liberar_cola(cola_aux);
		}
	}

	queue_iterate(self, _add_if_apply);

	return cola_aux;
}

int queue_count_satisfying(t_queue* self, bool(*condition)(void*)){
	t_queue *satisfying = queue_filter(self, condition);
	int result = satisfying->elements->elements_count;
//	liberar_cola(satisfying);
	return result;
}

bool queue_any_satisfy(t_queue* self, bool(*condition)(void*)){
	return (queue_count_satisfying(self, condition) > 0);
}



