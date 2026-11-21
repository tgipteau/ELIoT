CC = clang




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


clean :
	rm *.o
