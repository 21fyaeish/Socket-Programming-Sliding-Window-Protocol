#Define targets
all: client

#Compile client
client: client.c
	gcc -o client client.c

#Clean up generated files
clean:
	rm -f client
