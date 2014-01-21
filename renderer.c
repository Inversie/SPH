#include "circles_gl.h"
#include "stdio.h"
#include "mpi.h"
#include "communication.h"
#include "fluid.h"
#include <string.h> 
#include <stdlib.h>

void start_renderer()
{
    // Setup initial OpenGL ES state
    STATE_T state;
    memset(&state, 0, sizeof(STATE_T));

    // Start OpenGL
    init_ogl(&state.gl_state);

    // Create OpenGL buffers
    create_buffers(&state);

    // Create and set shaders
    create_shaders(&state);

    // Number of processes
    int num_compute_procs;
    int num_procs;
    MPI_Comm_size(MPI_COMM_WORLD, &num_compute_procs);
    num_compute_procs = num_procs - 1;

    // Allocate array of paramaters, render node space allocated
    // So we can use MPI_Gather instead of MPI_Gatherv
    param *params = malloc(num_procs*sizeof(param));
    param *compute_params = &params[1];

    // Receive initial paramaters
    MPI_Gather(MPI_IN_PLACE, 0, Paramtype, params, 1, Paramtype, 0, MPI_COMM_WORLD);

    // Allocate particle receive array
    int max_particles = compute_params[0].number_fluid_particles_global;
    int num_coords = 2;
    float *positions = (float*)malloc(num_coords * max_particles*sizeof(float));

    // Allocate points array(position + color);
    int point_size = 5 * sizeof(float);
    float *points = (float*)malloc(point_size*max_particles);

    int i,j, coords_recvd, disp;
    MPI_Status status;

    // Perhaps the RECV loop will help pipeline particle send and draw more than a gather
    while(1){

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

	for(i=0; i<num_compute_procs; i++) {
	    // receive particles
	    MPI_Recv(positions, max_particles, MPI_FLOAT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_FLOAT, &coords_recvd);
	    coords_recvd/=2;

            // Create proper points array
            for(j=0; j<coords_recvd; j++) {
	        points[j*5]   = positions[j*2]/10.0 - 1.0; 
	        points[j*5+1] = positions[j*2+1]/10.0 - 0.8;
                points[j*5+2] = 0.0;
	        points[j*5+3] = 0.0;
		points[j*5+4] = 1.0;
	
            }

  	    // Render particles
            update_points(points, coords_recvd, &state);
        }

	// Swap front/back buffers
        swap_ogl(&state.gl_state);
    };

    exit_ogl(&state.gl_state);

}