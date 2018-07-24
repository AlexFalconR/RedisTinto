#!/bin/bash
#script entrega

cd $HOME
git clone https://github.com/sisoputnfrba/tp-2018-1c-La-Cuarta-Es-La-Vencida

#———instalar redisLib—————————————
cd tp-2018-1c-La-Cuarta-Es-La-Vencida/RedisLib/
gcc -c -Wall -Werror -fpic sockets.c
gcc -c -Wall -Werror -fpic lib.c
gcc -shared -o libRedisLib.so sockets.o lib.o
sudo cp /home/utnso/tp-2018-1c-La-Cuarta-Es-La-Vencida/RedisLib/libRedisLib.so /usr/lib
sudo cp -R /home/utnso/tp-2018-1c-La-Cuarta-Es-La-Vencida/RedisLib/ /usr/include/

#———instalar coordinador—————————————

cd ../Coordinador/src
gcc -Wall -o coordinador coordinador.c -lRedisLib -lcommons -lpthread

#———instalar planificador—————————————

cd ../../Planificador/src
gcc -c -Wall -fpic ManejoColas.c
gcc -c -Wall -fpic Consola.c
gcc -shared -o libshared.so ManejoColas.o Consola.o -lreadline
gcc -L. -Wall -o planificador Planificador.c -lRedisLib -lcommons -lpthread -lshared

#———instalar instancia—————————————

cd ../../Instancia/src/
gcc -L. -Wall -o instancia Instancia.c -lRedisLib -lcommons -lpthread

#———instalar Esi—————————————

cd ../../Esi/src/
gcc -L. -Wall -o esi Esi.c parser.c -lRedisLib -lcommons -lpthread
