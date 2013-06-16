compile:	test

OBJS=	log.o tcio_file.o tcio_distributed.o tcio.o \
		config.o 


#Lib= -llmpe -lmpe
Lib = -lm
.c.o:
#	mpicc -DDEBUG -c $< -o $@ $(Lib)
	mpicc -DINFO -c $< -o $@ $(Lib)
#	mpicc -DINFO -DSEPERATE -c $< -o $@ $(Lib)
#	mpicc -DDEBUG -DSEPERATE -c $< -o $@ $(Lib)

test:	$(OBJS)
#	mpicc -DDEBUG -o $@ $? tcio_test.c
#	mpicc -DINFO -o $@ $? tcio_test.c $(Lib)
#	mpicc -DINFO -DSEPERATE -o $@ $? tcio_test.c $(Lib)
#	mpicc -DDEBUG -DSEPERATE -o $@ $? tcio_test.c $(Lib)
#	mpicc -DDEBUG -o $@ $? tcio_cube_test.c
	mpicc -DINFO -DSEPERATE -o $@ $? multi_array_test.c $(Lib)
clean:
	rm *.o test
	
