/**
    @file secret~
    @brief a delay effect
    @author Thomas Garvey
    @version 1.0
 
Big Picuture
    - Make functional delay
    - Pitch shift individual delays from the feedback
    - Stereo?
    - Slowing it down to something atmospheric?
 
TODO
    - Inputs for feedback and delay time
    - Pitch shift delays
        -  First: Figure out pitch shifting
        - Second: Write individual delays into new buffer?
*/

#include "ext.h"			// standard Max include, always required (except in Jitter)
#include "ext_obex.h"		// required for "new" style objects
#include "z_dsp.h"			// required for MSP objects
#include "ext_buffer.h"     // required for including buffers
#include "math.h"



// struct to represent the object's state
typedef struct _secret {
	t_pxobject	ob;
    t_sample    * b_buffer;
    long        buffer_size;
    long        write_loc;
    long        read_loc;
    double      feedback;
    float       b_sample_rate;
    //long        delay_length;
    
} t_secret;


// method prototypes
void *secret_new(t_symbol *s, long argc, t_atom *argv);
void secret_free(t_secret *x);
void secret_assist(t_secret *x, void *b, long m, long a, char *s);
void secret_dsp64(t_secret *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void secret_perform64(t_secret *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);


// global class pointer variable
static t_class *secret_class = NULL;


//***********************************************************************************************

void ext_main(void *r)
{
	t_class *c = class_new("secret~", (method)secret_new, (method)secret_free, (long)sizeof(t_secret), NULL, 0);
    class_dspinit(c);
    
	class_addmethod(c, (method)secret_dsp64,		"dsp64",	A_CANT, 0);
	class_addmethod(c, (method)secret_assist,	"assist",	A_CANT, 0);

	class_register(CLASS_BOX, c);
	secret_class = c;
    
}


void *secret_new(t_symbol *s, long argc, t_atom *argv)
{
    t_secret *x = (t_secret *)object_alloc(secret_class);

	if (x) {
		dsp_setup((t_pxobject *)x, 1);	// MSP inlets: arg is # of inlets and is REQUIRED!
		outlet_new((t_object *)x, "signal"); 		// signal outlet (note "signal" rather than NULL)
        post("dsp set up properly");
	}
    
    //Initializing the delay states
    x->b_sample_rate = sys_getsr();
   // x->delay_length = 22050; // needs error checking to not be greater than buffer size or less than 0
    x->buffer_size = 44100;
    x->feedback = 0.5;
    x->write_loc = 0;
    x->read_loc = x->write_loc - 22050; //(x->b_sample_rate/2);
    
    if(x->read_loc < 0){
        x->read_loc += x->buffer_size;
    }
    
    post("Buffer Sample Rate: %f", x->b_sample_rate);
    
    
    x->b_buffer = (t_sample *)sysmem_newptr(x->buffer_size * sizeof(t_sample)); // allocating memory for the buffer with each new instance of secret
                                                                                // it needs to be a pointer to a t_sample. sysmem_newptr is how to allocate memory for pointer in max systems --> the amount allocated needs to be the size of the buffer cut up into sample sizes (i.e. t_samples)
                                                                                //THIS DYNAMICALLY ALLOCATES MEMORY TO A POINTER...WHICH WE ARE GOING TO TREAT LIKE AN ARRAY
    
    if(x->b_buffer){
        for (int i=0; i < x->buffer_size; i++){
            x->b_buffer[i] = 0.0;
        }
    }
    
    if (x->b_buffer == NULL) {  //error checking to make sure buffer was made
        object_free(x);
        post("buffer not found");
        return NULL;
        }
    
	return (x);
}



void secret_free(t_secret *x)
{
    dsp_free((t_pxobject *)x);
    if (x->b_buffer) {
            sysmem_freeptr(x->b_buffer);
        }
    post("object memory freed");
}


void secret_assist(t_secret *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
		sprintf(s, "HELLO I am inlet %ld", a);
	}
	else {	// outlet
		sprintf(s, "I am outlet %ld", a);
	}
}

// registers a function for the signal chain in Max
void secret_dsp64(t_secret *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
	object_method(dsp64, gensym("dsp_add64"), x, secret_perform64, 0, NULL);
}


// this is the 64-bit perform method audio vectors
void secret_perform64(t_secret *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{

    double *inL = ins[0];  // Inlet
    double *outL = outs[0];  // Outlet
    long n = sampleframes;
    
    while(n--){
  
            x->b_buffer[x->write_loc] = *inL + (x->b_buffer[x->read_loc] * x->feedback);
            *outL = x->b_buffer[x->read_loc] ; // + *inL;
            outL++;
            inL++;
            
            if(x->write_loc >= x->buffer_size){
                x->write_loc = 0;
            }
            else{
                (x->write_loc)++;
            }
            if(x->read_loc >= x->buffer_size){
                x->read_loc = 0;
            }
            else{
                (x->read_loc)++;
            }
    }
        
}
    
    

