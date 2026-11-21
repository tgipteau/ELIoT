CC = clang


#ELIOT
eliot : eliot.o _ressources/mongoose.o _ressources/cJSON.o
	clang -o eliot eliot.o _ressources/mongoose.o _ressources/cJSON.o
eliot.o : eliot.c
	clang -c eliot.c -o eliot.o

# RESSOURCES
mongoose.o : _ressources/mongoose.c _ressources/mongoose.h
	$(CC) -c _ressources/mongoose.c -o _ressources/mongoose.o

cJSON.o : _ressources/cJSON.c _ressources/cJSON.h
	$(CC) -c _ressources/cJSON.c -o _ressources/cJSON.o

# UTILS
clean :
	rm *.o
