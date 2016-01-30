******
*  readMe.txt - for programming assignment 3
*  Author: Ming Rutar
*
*   This is a Graduate Level Network Programming Project. 
*   The description of the assignment is in pa3.pdf.
* 
******
includes Files:
	common.h
	common.cpp
	Coor.cpp		- coordinator
	data.cpp		- routers configuration file
	Makefile
	output.txt
	pa3_main.cpp	- pa3
	routing.h
	routing.cpp
	test-input.txt
To run:
	For coordinator: 	coor 
	For AS:	   	pa_3 n coorHost coorPort
			   n - the AS id, 
			   coorHost - coordinator's host
			   coorPort- coordinator's port
Platform:
	1) Windows.
	2) Linux.
	* the program uses C++ STL (standard template library). Although it is
	   C++ standard, it may vary from difference compilers.
Status:
	1) routing inside AS is complete.
	2) because I couldn't find the cost of  L1, L2,.. so, I didn't put bolder router
	    of neibour in routing table. It should be fairly easy.
	3) all commands are implemented; C, D, P, S are tested. F and U cann't be tested
 	    because of reason 2.
	4) The design accommodates BGP, but it is not really implemented yet.
	5) The output file for AS2 illustrates the forward table change after cost 
	    between R1 and R3 changed to 2, But there is a bug in distance vector
	    algorithm. Not all FT makes sense and I run out time to fix it.
	6) Because of the bug, for AS0 send msg sometime causes the msg bounce
	    between 2 routers.
	7) code for read input file is implemented, but it is hard to coordinate with the 
	    ASs's start time. So it not be used. The input file included in zip is a copy of
  	    input command. The output.txt does not corresponding to the commands.
	8) Each time pa_3 starts, it creates an output file ASn.txt for printing forward table 
	    and send msg. I copy/paste the text from all ASs.txt into the output.txt.
	9) 3 extra command for convenience:
	     Q - for quit. The coordinator sends msg to all ASs. All processes exit.
	     V <as id> <router id> - dump distance vector context. The cmd is hidden.
	     Z <as id> <router id> - dump neighbour context. The cmd is hidden.

