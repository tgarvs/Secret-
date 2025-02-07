/**
    @file secret~
    @brief a delay effect
    @author Thomas Garvey
    @version 2.0
 
TODO
    - Inputs for feedback and delay time and activation of delay elements
    - Keep messed up high pitch or "fix" it? --> how can I fully fix it?
    - add flutter delay
    - Add boolean switch to allow feedback for low_pitch
    - write current output into final buffer and reoutput --> crazy feedback? Cool atmosphere?
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
    
    
    t_sample    * b_buffer_high;
    long        buffer_size_high;
    long        write_loc_high;
    long        read_loc_high;
    double      feedback_high;
    float       b_sample_rate_high;
    
    t_sample    * b_buffer_low;
    long        buffer_size_low;
    long        write_loc_low;
    long        read_loc_low;
    double      feedback_low;
    float       b_sample_rate_low;
    Boolean     switch_low;
    
    
    
    //long        delay_length;
    
} t_secret;


// method prototypes
void *secret_new(t_symbol *s, long argc, t_atom *argv);
void secret_free(t_secret *x);
void secret_assist(t_secret *x, void *b, long m, long a, char *s);
void secret_dsp64(t_secret *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void secret_perform64(t_secret *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
double normal_delay (t_secret *x, double *inL, long n);
double high_pitch (t_secret *x, double *inL, long n);
double low_pitch(t_secret *x, double *inL, long n);
double random_tones(t_secret *x, double *inL, long n);


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
    
//Initializing delay buffer
    x->b_sample_rate = sys_getsr();
   // x->delay_length = 22050; // needs error checking to not be greater than buffer size or less than 0
    x->buffer_size = 44100*5;
    x->feedback = 0.9;
    x->write_loc = 0;
    x->read_loc = x->write_loc - 44100*4.7; //(x->b_sample_rate/2);
    
    if(x->read_loc < 0){
        x->read_loc += x->buffer_size;
    }
    
    x->b_buffer = (t_sample *)sysmem_newptr(x->buffer_size * sizeof(t_sample));
    
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
    

//Initializing high buffer
    x->b_sample_rate_high = sys_getsr();
    x->buffer_size_high = 44100*10;
    x->feedback_high = 0.999;
    x->write_loc_high = 0;
    x->read_loc_high = x->write_loc_high - 44099*10;
    
    if(x->read_loc_high < 0){
        x->read_loc_high += x->buffer_size_high;
    }

    x->b_buffer_high = (t_sample *)sysmem_newptr(x->buffer_size_high * sizeof(t_sample));
 
    
    if(x->b_buffer_high){
        for (int i=0; i < x->buffer_size_high; i++){
            x->b_buffer_high[i] = 0.0;
        }
    }
    
    if (x->b_buffer_high == NULL) {  //error checking to make sure buffer was made
        object_free(x);
        post("buffer not found");
        return NULL;
        }
    else{
        post("high buffer created");
    }
    
    
//Initializing low buffer
        x->b_sample_rate_low = sys_getsr();
        x->buffer_size_low = 44100*10;
        x->feedback_low = 0.999;
        x->write_loc_low = 0;
        x->read_loc_low = x->write_loc_low - 44099*10;
        x->switch_low = true;
        
        if(x->read_loc_low < 0){
            x->read_loc_low += x->buffer_size_low;
        }

        x->b_buffer_low = (t_sample *)sysmem_newptr(x->buffer_size_low * sizeof(t_sample));
     
        
        if(x->b_buffer_low){
            for (int i=0; i < x->buffer_size_low; i++){
                x->b_buffer_low[i] = 0.0;
            }
        }
        
        if (x->b_buffer_low == NULL) {  //error checking to make sure buffer was made
            object_free(x);
            post("buffer not found");
            return NULL;
            }
        else{
            post("low buffer created");
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

//    double *inL = ins[0];  // Inlet
//    double *outL = outs[0];  // Outlet
//    long n = sampleframes;
//
//    while(n--){
//
//            x->b_buffer[x->write_loc] = *inL + (x->b_buffer[x->read_loc] * x->feedback);
//            *outL = x->b_buffer[x->read_loc] ; // + *inL;
//            outL++;
//            inL++;
//
//            if(x->write_loc >= x->buffer_size){
//                x->write_loc = 0;
//            }
//            else{
//                (x->write_loc)++;
//            }
//            if(x->read_loc >= x->buffer_size){
//                x->read_loc = 0;
//            }
//            else{
//                (x->read_loc)++;
//            }
//    }
    
//double *inL = ins[0];  // Inlet
//double *outL = outs[0];  // Outlet
//long n = sampleframes;
//
//
//    while(n--){
//
//        x->b_buffer[(x->write_loc)] = *inL;// + (x->b_buffer[x->read_loc] * x->feedback);
////        x->b_buffer[(x->write_loc)+1] = *inL + (x->b_buffer[x->read_loc+1] * x->feedback);
//
//            if((x->write_loc) >= x->buffer_size){
//                (x->write_loc) = 0;
//            }
//            else{
//                (x->write_loc)++;
//            }
//
//        x->b_buffer[(x->write_loc)] = *inL;// + (x->b_buffer[x->read_loc] * x->feedback);
//
//
//            if((x->write_loc) >= x->buffer_size){
//                (x->write_loc) = 0;
//            }
//            else{
//                (x->write_loc)++;
//            }
//
//        *outL = x->b_buffer[x->read_loc] ; // + *inL;
//        outL++;
//        inL++;
//
//
//            if(x->read_loc >= x->buffer_size){
//                x->read_loc = 0;
//            }
//            else{
//                (x->read_loc)++;
//            }
//    }

    
//high pitch
    
//        double *inL = ins[0];  // Inlet
//        double *outL = outs[0];  // Outlet
//        long n = sampleframes;
//
//        while(n--){
//
//                x->b_buffer[x->write_loc] = *inL + (x->b_buffer[x->read_loc] * x->feedback);
//                *outL = x->b_buffer[x->read_loc] ; // + *inL;
//                outL++;
//                inL++;
//
//                if(x->write_loc >= x->buffer_size){
//                    x->write_loc = 0;
//                }
//                else{
//                    (x->write_loc)++;
//                }
//                if(x->read_loc >= x->buffer_size){
//                    x->read_loc = 0;
//                }
//                else{
//                    (x->read_loc)+=2;
//                }
//        }
    
    double *inL = ins[0];  // Inlet
    double *outL = outs[0];  // Outlet
    long n = sampleframes;

    while(n--){

//        *outL = normal_delay(x, inL, n) + high_pitch(x, inL, n) + low_pitch(x, inL, n);
        
//        normal_delay(x, inL, n);
        

        *outL = high_pitch(x, inL, n); //low_pitch(x, inL, n) + random_tones(x, inL, n);

        outL++;
        inL++;

    }
    
}

double normal_delay (t_secret *x, double *inL, long n)
{
    x->b_buffer[x->write_loc] = *inL + (x->b_buffer[x->read_loc] * x->feedback);

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

    return (x->b_buffer[x->read_loc]);
}


double high_pitch (t_secret *x, double *inL, long n)
{
    x->b_buffer_high[x->write_loc_high] = *inL + (x->b_buffer_high[x->read_loc_high] * x->feedback_high);
    
        if(x->write_loc_high >= x->buffer_size_high){
            x->write_loc_high = 0;
        }
        else{
            (x->write_loc_high)++;
        }
    
        if(x->read_loc_high >= x->buffer_size_high){
            x->read_loc_high = 0;
        }
        else{
            (x->read_loc_high)+=2;
        }
    
// Fixes decay into noise --> potential for granulation here with random function
    if(x->read_loc_high == x->write_loc_high){
        x->write_loc_high++;// = rand() % (x->buffer_size + 1);
    }
    
    return (x->b_buffer_high[x->read_loc_high]);
}


double low_pitch(t_secret *x, double *inL, long n){

//TODO: ADD BOOLEAN SWITCH


    x->b_buffer_low[(x->write_loc_low)] = *inL + (x->b_buffer[x->read_loc_low] * x->feedback_low);

//        if((x->write_loc_low) >= x->buffer_size_low){
//            (x->write_loc_low) = 0;
//        }
//        else{
//            (x->write_loc_low)++;
//        }
    
    if((x->write_loc_low) < x->buffer_size_low){
        (x->write_loc_low)++;
    }
    else{
        x->write_loc_low = x->buffer_size_low;
    }

    x->b_buffer_low[(x->write_loc_low)] = *inL + (x->b_buffer_low[x->read_loc_low] * x->feedback_low);

//
//        if((x->write_loc_low) >= x->buffer_size_low){
//            (x->write_loc_low) = 0;
//        }
//        else{
//            (x->write_loc_low)++;
//        }
    
    
    if((x->write_loc_low) < x->buffer_size_low){
        (x->write_loc_low)++;
    }
    else{
        x->write_loc_low = x->buffer_size_low;
    }


        if(x->read_loc_low >= x->buffer_size_low){
            x->read_loc_low = 0;
        }
        else{
            (x->read_loc_low)++;
        }

    return (x->b_buffer_low[x->read_loc_low]);
}


//
//double random_tones(t_secret *x, double *inL, long n)
//{
//    int random_buffer_loc = rand() % (x->buffer_size + 1);
//
//    for(int i = 0; i < 1000; i++){
//        random_buffer_loc++;
//    }
//
//    return (x->b_buffer[random_buffer_loc]);
//}
