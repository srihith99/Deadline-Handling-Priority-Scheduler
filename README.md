# Deadline-Handling-Priority-Scheduler

A simulator written in C++ that schedules arriving cyclic processes by considering their priorities and ensuring each process cycle is scheduled before it's deadline . RMS and EDF algorithms were used.

Compilation :

	rm scheduling :
	g++ RMS.cpp -o RMS

	edf scheduling :
	g++ EDF.cpp -o EDF

Note : Before execution , ensure the file 'inp-params.txt' is present in the the same directory as that of the executable

Execution :
	
	rm scheduling :
	./RMS

	edf scheduling :
	./EDF

Note : Output shall be written to corresponding files

