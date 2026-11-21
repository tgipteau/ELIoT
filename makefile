CC = clang


#ELIOT
eliot : eliot.o _ressources/mongoose.o _ressources/cJSON.o
	clang -o eliot eliot.o _ressources/mongoose.o _ressources/cJSON.o
eliot.o : eliot.c
	clang -c eliot.c -o eliot.o


# BROKER
broker : broker.o _ressources/mongoose.o
	clang -o broker broker.o _ressources/mongoose.o
broker.o : broker.c
	clang -c broker.c -o broker.o

# BRIDGE
bridge : bridge.o _ressources/mongoose.o
	$(CC) -o bridge bridge.o _ressources/mongoose.o

bridge.o : bridge.c _ressources/mongoose.h
	$(CC) -c bridge.c -o bridge.o


mongoose.o : _ressources/mongoose.c _ressources/mongoose.h
	$(CC) -c _ressources/mongoose.c -o _ressources/mongoose.o

cJSON.o : _ressources/cJSON.c _ressources/cJSON.h
	$(CC) -c _ressources/cJSON.c -o _ressources/cJSON.o

clean :
	rm *.o
