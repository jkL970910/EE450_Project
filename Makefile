CC=g++
CFLAGS = -Wall -Werror -std=c++11

all: compile_scheduler compile_hospitalA compile_hospitalB compile_hospitalC compile_client

scheduler: compile_scheduler
	@./scheduler

hospitalA: compile_hospitalA
	@./hospitalA

hospitalB: compile_hospitalB
	@./hospitalB

hospitalC: compile_hospitalC
	@./hospitalC	

client: compile_client
	@./client

compile_scheduler: scheduler.cpp
	@$(CC) $(CFLAGS) -o scheduler $^

compile_hospitalA: hospitalA.cpp
	@$(CC) $(CFLAGS) -o hospitalA $^

compile_hospitalA: hospitalB.cpp
	@$(CC) $(CFLAGS) -o hospitalB $^

compile_hospitalA: hospitalC.cpp
	@$(CC) $(CFLAGS) -o hospitalC $^

compile_client: client.cpp
	@$(CC) $(CFLAGS) -o client $^


clean:
	rm -rf *.o scheduler hospitalA hospitalB hospitalC client