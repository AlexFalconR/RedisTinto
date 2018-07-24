/*
 ============================================================================
 Name        : Instancia.c
 Author      : FedricoJoel
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "instancia.h"

pthread_mutex_t mutex_memoria = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
	ruta_config = string_duplicate(argv[1]);
	levantar_logger();
	parsear_config();
	circular=0;
	tabla_entradas = dictionary_create();
	conectarse_coord();
	pthread_create(&t_dump,NULL,(void*)&dump, NULL);


	//tam_entradas = 5;
	//cant_entradas= 5;



	//inicializo estructuras
	int lng = dictionary_size(tabla_entradas);
	array_lru = calloc(cant_entradas,sizeof(int));
//	array_entradas = calloc(cant_entradas,41 * sizeof(char));
	array_entradas = calloc(cant_entradas,sizeof(char*));
	printf("el array es %s \n",array_entradas[0]);
	entradas = calloc(cant_entradas,tam_entradas*sizeof(char));
	printf("largo es %d \n", lng);

	//mock
	//set_key_value("A","AAAAAAAA");
	//	set_key_value("B","BBBBB");
	//	set_key_value("C","CCCCCCC");
	//	set_key_value("D","DDDD");
//	set_key_value("D","DDDD");

//	set_key_value("A","Av2");
//	set_key_value("C","Cv2");
//	set_key_value("D","DDDDDDD");
//	set_key_value("apolo","hola");
//	set_key_value("joaquin","de3");
//	set_key_value("lean","esdeocho");
//	dump();
//	char* prueba = leer_archivo("apolo.txt");
//	recuperar_estado();
//	store_key("lean");

//	dictionary_put(tabla_entradas,"apolo", entrada_create(0,4));
//	dictionary_put(tabla_entradas,"joaquin", entrada_create(3,3));
//	dictionary_put(tabla_entradas,"lean", entrada_create(1,8));
//	array_entradas[0]="apolo";
//	array_entradas[1]="lean";
//	array_entradas[2]="lean";
//	array_entradas[3]="joaquin";

//	set_key_value("A","apolo");
//	set_key_value("lean","lalalalalalala");
//	set_key_value("el3","esde3carat");
//	lng = dictionary_size(tabla_entradas);
//	printf("largo es %d \n",lng);
//	mostrar_dic();

	//printf(config);
	//pthread_create(&t_dump,NULL,(void*)&dump, NULL);
	atender_coord();

	return EXIT_SUCCESS;
}

void liberar_recursos(){
	//TODO
//	liberar_array();
	liberar_diccionario();
	free(IP_COORDINADOR);
	free(ALG_REEMPLAZO);
	free(PUNTO_MONTAJE);
	free(NOMBRE);
	free(ruta_config);
}

void liberar_diccionario(){
	if (dictionary_size(tabla_entradas) > 0){
		dictionary_clean_and_destroy_elements(tabla_entradas,(void*) entrada_destroy);
	}
	dictionary_destroy(tabla_entradas);

}

void liberar_array(){
	for(int pos = 0; pos < cantidadElementos(array_entradas); pos++){
		free(array_entradas[pos]);
	}
	free(array_entradas);
}

int cantidadElementos(char ** array) {
	size_t count = 0;
	while (array[count] != NULL)
		count++;
	return count;
}

void atender_coord(){
	while(1){
		int proto = recibirProtocolo(s_coordinador);
		switch(proto) {
			case SET_KEY_INST  :
			{
				pthread_mutex_lock(&mutex_memoria);
				log_info(logger,"accion recibida: SET");
				char* key = esperarMensaje(s_coordinador);
				log_info(logger,"clave recibida para SET: %s",key);
				char* value = esperarMensaje(s_coordinador);
				log_info(logger,"valor recibido para SET: %s",value);
				set_key_value(key,value);
//				actualizar_lru(key);
				pthread_mutex_unlock(&mutex_memoria);
				enviarProtocolo(s_coordinador,OK_OP);
				free(key);
				free(value);
				break;
			}
			case STORE_KEY_INST :
			{
				pthread_mutex_lock(&mutex_memoria);
				log_info(logger,"accion recibida: STORE");
				char* key = string_duplicate(esperarMensaje(s_coordinador));
				log_info(logger,"clave recibida para STORE: %s",key);
				store_key(key);
//				actualizar_lru(key);
				pthread_mutex_unlock(&mutex_memoria);
				enviarProtocolo(s_coordinador,OK_OP);
				free(key);
				break;
			}
			case DN_KEEPALIVE: // @suppress("Symbol is not resolved")
			{	enviarProtocolo(s_coordinador,DN_ALIVE); // @suppress("Symbol is not resolved")
				break;
			}
			case COMPACTAR_INST: // @suppress("Symbol is not resolved")
			{
				compactacion();
				break;
			}
			case -1:	//DESCONEXION DEL COORDINADOR
			{
				log_info(logger,"Se desconecto el Coordinador, por lo tanto de abortará la instancias");
				exit_gracefully(0);
			}
		}
	}
}

void actualizar_lru(char* clave){
	dictionary_iterator(tabla_entradas, (void*)sumar_uno_lru);
	t_entrada *aux = dictionary_get(tabla_entradas, clave);
	aux->uso = 0;
	mostrar_dic();

}

void sumar_uno_lru(char* clave, t_entrada* entrada){
	t_entrada *aux = dictionary_get(tabla_entradas, clave);
	aux->uso=(aux->uso +1);
//	dictionary_put(tabla_entradas,clave, entrada_create(aux->numero_entrada,aux->longitud,(aux->uso + 1)));
}

void mostrar_dic(){
	dictionary_iterator(tabla_entradas, (void*)mostrar_elem);
}

void mostrar_elem(char* clave, t_entrada* entrada){
	log_info(logger,"clave: %s",clave);
	log_info(logger,"numero_ent: %d , longitud: %d , uso: %d \n",entrada->numero_entrada, entrada->longitud, entrada->uso);
}

void dump_elem(char* clave){
	store_key(clave);
}

void dump(){
	while(1) {
		sleep(INTERVALO);
		//pthread_mutex_lock(&mutex_memoria);
		enviarProtocolo(s_coordinador, START_DUMP);
		log_info(logger, "Comienza la ejecución del dumppo por tiempo transcurrido");
		dictionary_iterator(tabla_entradas,(void*)dump_elem);
		log_info(logger, "Se terminó de ejecutar el dump");
		enviarMensaje(s_coordinador, "Stop_Dump");
		//enviarProtocolo(s_coordinador, STOP_DUMP);
		//pthread_mutex_unlock(&mutex_memoria);
	}

}

int set_key_value(char* key, char* value){

	if (dictionary_has_key(tabla_entradas,key) == true){ //MODIFICACION

		//1 - Elimino del dic
		t_entrada *aux = dictionary_remove(tabla_entradas, key);
		int longitud = string_length(value);
		int c_entradas = cantidad_entradas(longitud);
		if (c_entradas > cantidad_entradas(aux->longitud)){
			//TODO ESTO DA UN PUTO ERROR
			dictionary_put(tabla_entradas,key, entrada_create(aux->numero_entrada,aux->longitud,0));
			mostrar_dic();
			return -1;
		}

		//2 - Reasigno entrada, si hace falta lo borro el array.
		int primer_entrada = re_asignar_entrada(longitud,aux->numero_entrada,aux->longitud,key);
		str_array_print(array_entradas,cant_entradas);

		//3 - Agrego al dictionary
		dictionary_put(tabla_entradas,key, entrada_create(primer_entrada,longitud,0));
		mostrar_dic();

		//4 - Agrego al array
		int i;
		for (i=0;i < c_entradas;i++){
			array_entradas[primer_entrada+i]=key;
		}
		str_array_print(array_entradas,cant_entradas);

		//5 - agrego a memoria
		escribir_entrada(value,primer_entrada);
		actualizar_lru(key);
		return 1;
	}
	else{ //ALTA

		//1 - Agrego al dictionary
		int longitud = string_length(value);
		int primer_entrada = seleccionar_entrada(longitud);
		int c_entradas = cantidad_entradas(longitud);
		dictionary_put(tabla_entradas, key, (void*) entrada_create(primer_entrada, longitud,0));
		mostrar_dic();
		int i;

		//2 - Agrego al array
		for (i=0;i < c_entradas;i++){
			array_entradas[primer_entrada+i]=key;
		}

		log_info(logger,"El array de entradas es:");
		str_array_print(array_entradas,cant_entradas);

		//3 - Agrego a la memoria
		escribir_entrada(value,primer_entrada);
		char* valor_entrada = leer_entrada(key);
		log_info(logger,"La entrada se escribio correctamente \n");
		log_info(logger,"la entrada escrita es: %s \n",valor_entrada);
		actualizar_lru(key);
		free(valor_entrada);
		return 1;
	}
}

int str_array_size(char ** array){
	    size_t count = 0;
	    while (array[count] != NULL) count++;
	    return count;
}

void str_array_print(char ** array,int tam){
	int i;
	int size = tam;
		for (i=0;i<size;i++){
			log_info(logger,"El argumento de la posicion %d es '%s'\n",i,array[i]);
		}
}

int seleccionar_entrada(int largo){
	int hay_espacio=0;
	//int compactar =0;
	char* clave_reemplazar;
	while(hay_espacio == 0){
		int lugar_vacio = buscar_espacio_vacio(largo);
		int lugar_vacio_contiguo = buscar_espacio_vacio_contiguo(largo);
		if (lugar_vacio_contiguo != -1){ //si hay vacios contiguos retorno
			lugar_vacio=1;
			return lugar_vacio_contiguo;
		}else if(lugar_vacio != -1){ //si hay vacios no contiguos compacto
			//TODO DESCOMENTAR
			enviarProtocolo(s_coordinador,NECESITA_COMP);
			int proto = recibirProtocolo(s_coordinador);
			compactacion();
			lugar_vacio_contiguo = buscar_espacio_vacio_contiguo(largo);
			lugar_vacio=1;
			return lugar_vacio_contiguo;
		}
		else //Reemplazo clave
		{
			clave_reemplazar = seleccionar_clave_reemplazar();
			borrar_clave(clave_reemplazar);
			str_array_print(array_entradas,cant_entradas);
		}
	}

}

char* seleccionar_clave_reemplazar(){

	int atomica =0;
	char*clave_reemplazar;
	char* clave_busqueda;

	bool is_in_clave(char* clave_lista) {
				if(string_equals_ignore_case(clave_lista,clave_busqueda)){
					clave_reemplazar = clave_busqueda;
					return true;
				}
				return false;
	}

	if (string_equals_ignore_case(ALG_REEMPLAZO,"CIRC")){
		do{
			clave_reemplazar = array_entradas[circular];
			atomica = es_atomica(clave_reemplazar);
			circular_siguiente();
		}while ((clave_reemplazar =='\0') || (atomica==0));
		return clave_reemplazar;
	}else if (string_equals_ignore_case(ALG_REEMPLAZO,"LRU")){ //LRU
		t_list* lista_claves_reemplazar = get_entrada_mayor_lru(tabla_entradas);
		if (list_size(lista_claves_reemplazar)==1){ // NO HAY EMPATE
			clave_reemplazar = (char*) list_get(lista_claves_reemplazar,0);
			return clave_reemplazar;
		}else{ //HAY EMPATE
			do{
				clave_busqueda = array_entradas[circular];
				if (list_any_satisfy(lista_claves_reemplazar,(void *)is_in_clave)){
					return clave_reemplazar;
				}
				circular_siguiente();
			}while (true);
		}
		return clave_reemplazar;
	}else if (string_equals_ignore_case(ALG_REEMPLAZO,"BSU")){ //BSU
		t_list* lista_claves_reemplazar = get_entrada_mayor_longitud(tabla_entradas);
		if (list_size(lista_claves_reemplazar)==1){ // NO HAY EMPATE
			clave_reemplazar = (char*) list_get(lista_claves_reemplazar,0);
			//TODO linea apocaliptica
//			liberar_lista(lista_claves_reemplazar);
			return clave_reemplazar;
		}else{ //HAY EMPATE
			do{
				clave_busqueda = array_entradas[circular];
				if (list_any_satisfy(lista_claves_reemplazar,(void *)is_in_clave)){
					//TODO
//					liberar_lista(lista_claves_reemplazar);
					return clave_reemplazar;
				}
				circular_siguiente();
			}while (true);
		}
		return clave_reemplazar;
	}
	return clave_reemplazar;
}

void liberar_lista(t_list* lista_claves_reemplazar){
	for (int pos = 0; pos < list_size(lista_claves_reemplazar); pos++){
		char* elemento = list_get(lista_claves_reemplazar, pos);
		free(elemento);
	}
	list_destroy(lista_claves_reemplazar);
}

t_list* get_entrada_mayor_lru(t_dictionary *self) {
	int mayor_uso = 0;
	t_list* lista_empatados = list_create();

	void buscar_mayor_uso(char* clave, t_entrada* entrada){
		if (entrada->longitud <= tam_entradas){
			if (entrada->uso > mayor_uso){
				mayor_uso = entrada->uso;
			}
		}
	}

	void listar_empates(char* clave, t_entrada* entrada){
		if (entrada->longitud <= tam_entradas){
			if (entrada->uso == mayor_uso){
				list_add(lista_empatados, clave);
			}
		}
	}

	dictionary_iterator(self, (void*)buscar_mayor_uso);
	dictionary_iterator(self, (void*)listar_empates);
	return lista_empatados;
}

t_list* get_entrada_mayor_longitud(t_dictionary *self) {
	int mayor_longitud = 0;
	t_list* lista_empatados = list_create();

	void buscar_mayor_longitud(char* clave, t_entrada* entrada){
		if (entrada->longitud <= tam_entradas){
			if (entrada->longitud > mayor_longitud){
				mayor_longitud = entrada->longitud;
			}
		}
	}

	void listar_empates(char* clave, t_entrada* entrada){
		if (entrada->longitud <= tam_entradas){
			if (entrada->longitud == mayor_longitud){
				char* aux = string_duplicate(clave);
				list_add(lista_empatados, aux);
			}
		}
	}

	dictionary_iterator(self, (void*)buscar_mayor_longitud);
	dictionary_iterator(self, (void*)listar_empates);
	return lista_empatados;
}

int es_atomica(char* clave){
	int i;
	int cont=0;
	for (i=0;i<=cant_entradas;i++){
		if (array_entradas[i]==clave){
			cont++;
		}
	}
	if (cont>1){
		return 0;
	}
	else{
		return 1;
	}
}


void circular_siguiente(){
	if (circular < (cant_entradas - 1)){
		circular++;
	}
	else{
		circular=0;
	}
}

int buscar_espacio_vacio_contiguo(int largo){
	int c_entradas = cantidad_entradas(largo);
		int i;
		int cont=0;
		int encontrado = 0;
		int ultima_entrada;
		//busco espacio vacio en el array
		for (i=0; i<cant_entradas;i++){
			if (array_entradas[i]==NULL){
				cont++;
			}else if (string_is_empty(array_entradas[i])){
				cont++;
			}
			else {
				cont = 0;
			}
			if (cont == c_entradas){
				encontrado=1;
				ultima_entrada = i;
				break;
			}
		}
		if (encontrado == 1){
			int primer_entrada = ultima_entrada - c_entradas + 1;
			return primer_entrada;
		}
		else {
			return -1;
		}
}

int buscar_espacio_vacio(int largo){
	int c_entradas = cantidad_entradas(largo);
		int i;
		int cont=0;
		int encontrado = -1;
		//busco espacio vacio en el array
		for (i=0; i<cant_entradas;i++){
			if (array_entradas[i]==NULL){
				cont++;
			}else if (string_is_empty(array_entradas[i])){
				cont++;
			}
			if (cont == c_entradas){
				encontrado=1;
				break;
			}
		}
		return encontrado;
}

void borrar_clave(char* clave){
	//borro la clave del array
	borrar_clave_array(clave);

	//dictionary_remove(tabla_entradas, clave);
	dictionary_remove_and_destroy(tabla_entradas, clave, (void*) entrada_destroy);
	mostrar_dic();
}

void borrar_clave_array(char* clave){
	int i;
		for (i=0;i<cant_entradas;i++){
			if (array_entradas[i] != NULL){
				if (string_equals_ignore_case(array_entradas[i],clave)){
					//free(array_entradas[i]);
					//calloc(41, sizeof(char));
					//array_entradas[i] = string_repeat('\0', 41);
					array_entradas[i]="\0";
					//memset(array_entradas[i],'\0',41);
				}
			}
		}
}

static void entrada_destroy(t_entrada *self){
	//free(&self->numero_entrada);
	//free(&self->longitud);
	free(self);
}

int cantidad_entradas(int largo){
	int resto = largo %tam_entradas;
	if (resto ==0){
		return largo/tam_entradas;
	}else{
		return (largo / tam_entradas)+1;
	}
}

int re_asignar_entrada(int largo_nuevo,int entrada, int largo, char* key){
	if (largo_nuevo == largo){
		return entrada;
	}else if(largo_nuevo < largo){
		borrar_clave_array(key);
		return entrada;
	}
	else {
		borrar_clave_array(key);
		return seleccionar_entrada(largo_nuevo);
	}
}

void store_key(char* key){
	char* valor = leer_entrada(key);
	if (!string_equals_ignore_case(valor, "Error, clave no encontrada")){
		if(stat(PUNTO_MONTAJE, &directorio) == -1)
			mkdir(PUNTO_MONTAJE, 0755);
		char* nuevo_archivo = string_duplicate(PUNTO_MONTAJE);
		string_append(&nuevo_archivo,"/");
		string_append(&nuevo_archivo,key);
		string_append(&nuevo_archivo,".txt");
		FILE *fp = fopen(nuevo_archivo, "w+");
		fputs(valor, fp);
		fclose(fp);
		free(nuevo_archivo);
	}else{
		log_info(logger, "La clave %s no se encuentra guardada", key);
	}
}

int recuperar_estado(){
	char* valor;
	char* clave;
	int longitud_nombre_archivo;
	DIR *dip;
	struct dirent   *dit;
	//int i = 0;
    if ((dip = opendir(PUNTO_MONTAJE)) == NULL){
    	perror("opendir");
	    return 0;
	}

	while ((dit = readdir(dip)) != NULL)
	{
	if ((strcmp(dit->d_name,".")!=0) && (strcmp(dit->d_name,"..")!=0)){
		//leo el contenido
		valor = leer_archivo(dit->d_name);

		//recorto la extension
		longitud_nombre_archivo = string_length(dit->d_name);
		clave =  string_substring_until(dit->d_name,longitud_nombre_archivo-4);

		//hago un set de la clave y el valor;
		set_key_value(clave,valor);
		mostrar_dic();
		}
	}

	if (closedir(dip) == -1)
	{
		perror("closedir");
		return 0;
	}

}

char* leer_archivo(char* nombre){
	char* buffer = calloc(tam_entradas,sizeof(char));
	FILE *fp;
	char* ruta = string_new();
	string_append(&ruta,PUNTO_MONTAJE);
	string_append(&ruta,"/");
	string_append(&ruta,nombre);
	fp = fopen(ruta, "r");
	if(fp == NULL) {
		perror("Error opening file");
		return("error");
	}
	fgets (buffer, 60, fp);
	log_info(logger,"el contenido es %s",buffer);
	fclose(fp);
	return buffer;
}

static t_entrada *entrada_create(int numero, int longitud,int lru){
	t_entrada *new = malloc( sizeof(t_entrada));
	new->numero_entrada = numero;
	new->longitud = longitud;
	new->uso = lru;
	return new;
}

void conectarse_coord(){
	log_info(logger, "Intentando conectar con el coordinador");
	int p_resp;
	s_coordinador = conectarAuth(PUERTO_COORDINADOR,IP_COORDINADOR,INST,&p_resp);
	if(p_resp == OKCONN){
		enviarMensaje(s_coordinador, NOMBRE);
		int i;
		for (i=0;i<2;i++){ //RECIBO 2 DATOS
			int proto = recibirProtocolo(s_coordinador);
			switch(proto) {
			   case TAM_ENTRADAS  :
			      tam_entradas = atoi(esperarMensaje(s_coordinador));
			      break;
			   case CANT_ENTRADAS  :
				  cant_entradas = atoi(esperarMensaje(s_coordinador));
			      break;
			}
		}
		log_info(logger,"Conectado a Coordinador con socket %d ",s_coordinador);
	}else
	{
		log_info(logger,"Error de coneccion");
	}
}

void parsear_config() {
	config = config_create(ruta_config);
	PUERTO_COORDINADOR = config_get_int_value(config,"PUERTO_COORD");
	IP_COORDINADOR = string_duplicate(config_get_string_value(config, "IP_COORD"));
	ALG_REEMPLAZO = string_duplicate(config_get_string_value(config, "ALG_REEMPLAZO"));
	PUNTO_MONTAJE = string_duplicate(config_get_string_value(config, "PUNTO_MONTAJE"));
	NOMBRE = string_duplicate(config_get_string_value(config, "NOMBRE"));
	INTERVALO = config_get_int_value(config,"INTERVALO");
	config_destroy(config);
	log_info(logger, "Configuracion levantada");
}

void levantar_logger() {
	logger = abrir_logger(ruta_log, programa, LOG_LEVEL_INFO);
	log_info(logger, "Logger iniciado");
}

//TODO ANTES DE INVOCAR EST FUNCION, INVOCAR "liberar_recursos();"
void exit_with_error(char* error_msg) {
	log_error(logger, error_msg);
	exit_gracefully(1);
}

//TODO ANTES DE INVOCAR EST FUNCION, INVOCAR "liberar_recursos();"
void exit_gracefully(int return_nr) {
  log_destroy(logger);
  liberar_recursos();
  exit(return_nr);
}

void escribir(char* mensaje,int offset){
	size_t mensaje_size = strlen(mensaje); // no escribe el \0
	memcpy(entradas+offset, mensaje, mensaje_size);
}

void escribir_entrada(char* mensaje, int entrada){
	escribir(mensaje,tam_entradas*entrada);
}

char* leer_entrada(char* key){
	//TODO: MODIFIQUE FUNCION "LONGITUD_CLAVE()"
	int longitud = longitud_clave(key);
	if (longitud != -1){
		int primer_entrada = primer_entrada_clave(key);
		char* buffer = calloc(longitud+1,sizeof(char));
		int offset = primer_entrada * tam_entradas;
		memcpy(buffer,entradas + offset, longitud);
		return buffer;
	}else{
		return "Error, clave no encontrada";
	}
}

void mostrar_memoria(){
	dictionary_iterator(tabla_entradas, (void*)mostrar_bloques_clave);
}

void mostrar_bloques_clave(char* clave, t_entrada* entrada){
	log_info(logger,"mem en clave %s: %s",clave,leer_entrada(clave));
}

int longitud_clave(char* key){
	if ( dictionary_has_key(tabla_entradas, key) == true){
		t_entrada *aux = dictionary_get(tabla_entradas, key);
		int longitud = aux->longitud;
		return longitud;
	}else{
		return -1;
	}
}

int primer_entrada_clave(char* key){
	t_entrada *aux = dictionary_get(tabla_entradas, key);
	int longitud = aux->numero_entrada;
	return longitud;
}

void compactacion() {
	log_info(logger,"Compactando");
	int i;
	char* entrada_anterior;
	char* entrada_actual;

	for(i=1; i < cant_entradas; i++) {//loop a partir del segundo
		entrada_actual = array_entradas[i];
		entrada_anterior = array_entradas[i-1];

		if(!(entrada_esta_vacia(entrada_actual))){ //actual tiene algo
			if(entrada_esta_vacia(entrada_anterior)) { //anterior esta vacia
				compactar_elemento_array(i,array_entradas);
			}
		}
	}
	mostrar_memoria();
	enviarProtocolo(s_coordinador,OK_COMPACTACION); // @suppress("Symbol is not resolved")
}


void compactar_elemento_array(int indice_elemento,char** array) {
	char* elemento_a_compactar;
	char* aux;
//	char* mem_ant;
	char* mem_act;
	t_entrada* entrada;
	do {
		// Sobreescribo el bloque
//		mem_ant = leer_entrada(array[indice_elemento - 1]);
		mem_act = leer_entrada(array[indice_elemento]);
		escribir_entrada(mem_act,(indice_elemento - 1));

		// Actualizo diccionario
		entrada = dictionary_get(tabla_entradas,array[indice_elemento]);
		entrada->numero_entrada--;

		// Invierto claves de el array de entradas
		elemento_a_compactar = array[indice_elemento];
		aux = array[indice_elemento - 1];
		array[indice_elemento - 1] = elemento_a_compactar;
		array[indice_elemento] = aux;

		indice_elemento--;
		free(mem_act);
	}while((indice_elemento - 1 >= 0) && (entrada_esta_vacia(array[indice_elemento - 1])));

}


bool entrada_esta_vacia(char* entrada) {
	if(entrada == NULL) {
		return true;
	}else if(string_is_empty(entrada)){
		return true;
	}
	return false;
}

