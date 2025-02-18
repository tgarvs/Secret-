
/**
    @file secret~
    @brief a multi delay effect
    @author Thomas Garvey
    @version 3.0
 
*/

#include "ext.h"            // standard Max include, always required (except in Jitter)
#include "ext_obex.h"        // required for "new" style objects
#include "z_dsp.h"            // required for MSP objects
#include "ext_buffer.h"     // required for including buffers
#include "math.h"


// Object struct. This struct holds all current states
typedef struct _secret {
// Normal delay parameters
    t_pxobject    ob;
    t_sample    * buffer_normal;
    long        buffer_size;
    long        write_loc;
    long        read_loc;
    double      feedback;
    float       b_sample_rate;
    
// High delay parameters
    t_sample    * buffer_high;
    long        buffer_size_high;
    long        write_loc_high;
    long        read_loc_high;
    double      feedback_high;
    float       b_sample_rate_high;

// Low delay parameters
    t_sample    * buffer_low;
    long        buffer_size_low;
    long        write_loc_low;
    long        read_loc_low;
    double      feedback_low;
    float       b_sample_rate_low;
    double      temp_low;
    Boolean     switch_low;
    
// Flutter delay parameters
    t_sample    * buffer_flutter;
    long        buffer_size_flutter;
    long        write_loc_flutter;
    long        read_loc_flutter;
    double      feedback_flutter;
    float       b_sample_rate_flutter;
    
// Random tones parameters
    long        random_number;
    long        random_number_temp;
    long        grain_size;
    long        random_number_two;
    long        random_number_temp_two;
    long        grain_size_two;
} t_secret;


// method prototypes
void *secret_new(t_symbol *s, long argc, t_atom *argv);
void secret_free(t_secret *x);
void secret_assist(t_secret *x, void *b, long m, long a, char *s);
void secret_dsp64(t_secret *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void secret_perform64(t_secret *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);
double normal_delay (t_secret *x, double inL, long n);
double high_pitch (t_secret *x, double inL, long n);
double low_pitch(t_secret *x, double inL, long n);
double random_tones(t_secret *x, double inL, long n);
double random_tones_offset(t_secret *x, double inL, long n);
double flutter_delay (t_secret *x, double inL, long n);

void secret_in2 (t_secret *x, long q);
void secret_in3 (t_secret *x, long q);


// global class pointer variable
static t_class *secret_class = NULL;


//***********************************************************************************************
/*
 @brief: Main function in Max that ties object and all methods back to CMake/Header Files/Libraries in the SDK kit
 @params: Void pointer
 @return: [void]
 */
void ext_main(void *r)
{
    t_class *c = class_new("secret~", (method)secret_new, (method)secret_free, (long)sizeof(t_secret), NULL, 0);
    class_dspinit(c);
    
    class_addmethod(c, (method)secret_dsp64,        "dsp64",    A_CANT, 0);
    class_addmethod(c, (method)secret_assist,    "assist",    A_CANT, 0);
    class_addmethod(c, (method)secret_in2, "in2", A_LONG, 0);
    class_addmethod(c, (method)secret_in3, "in3", A_LONG, 0);

    class_register(CLASS_BOX, c);
    secret_class = c;
    
}

/*
 @brief: Called when a new Secret object is created in the patcher. Initializes all parameters/inlets/outlets
 @params: Pointer to struct and whatever arguements are included in the Secret object (n/a for this object)
 @return: [void]
 */
void *secret_new(t_symbol *s, long argc, t_atom *argv)
{
    t_secret *x = (t_secret *)object_alloc(secret_class);
    intin(x, 2); // Max (non-MSP) inlets
    intin(x, 3);

    if (x) {
        dsp_setup((t_pxobject *)x, 1);    // MSP inlets: arg is # of inlets
        outlet_new((t_object *)x, "signal");         // signal outlets (note "signal" rather than NULL)
        outlet_new((t_object *)x, "signal");
        outlet_new((t_object *)x, "signal");
        outlet_new((t_object *)x, "signal");
        outlet_new((t_object *)x, "signal");
        outlet_new((t_object *)x, "signal");
        outlet_new((t_object *)x, "signal");
        post("dsp set up properly");
    }
  
//--------------------------------Normal Buffer Initialization-------------------------------------------------------------
    x->buffer_size = 44100*15;
    x->feedback = 0.5;
    x->write_loc = 0;
    x->read_loc = x->write_loc - 22050*15;
    
    if(x->read_loc < 0){
        x->read_loc += x->buffer_size;
    }
    
    x->buffer_normal = (t_sample *)sysmem_newptr(x->buffer_size * sizeof(t_sample));
    
    if(x->buffer_normal){
        for (int i=0; i < x->buffer_size; i++){
            x->buffer_normal[i] = 0.0;
        }
    }
    
    if (x->buffer_normal == NULL) {  //error checking to make sure buffer was made
        object_free(x);
        post("buffer not found");
        return NULL;
        }
    
//---------------------------------High Buffer Initialization------------------------------------------------------------
    x->buffer_size_high = 44100*40;
    x->feedback_high = 0.8;
    x->write_loc_high = 0;
    x->read_loc_high = x->write_loc_high - 44099*19;
    
    if(x->read_loc_high < 0){
        x->read_loc_high += x->buffer_size_high;
    }

    x->buffer_high = (t_sample *)sysmem_newptr(x->buffer_size_high * sizeof(t_sample));
 
    if(x->buffer_high){
        for (int i=0; i < x->buffer_size_high; i++){
            x->buffer_high[i] = 0.0;
        }
    }
    
    if (x->buffer_high == NULL) {  //error checking to make sure buffer was made
        object_free(x);
        post("buffer not found");
        return NULL;
        }


//---------------------------------Low Buffer Initialization------------------------------------------------------------
//Initializing low buffer
        x->buffer_size_low = 44100*30;
        x->feedback_low = 0.8;
        x->write_loc_low = 0;
        x->read_loc_low = x->write_loc_low - 44099*30;
        x->temp_low = 0;
        x->switch_low = true;
        
        if(x->read_loc_low < 0){
            x->read_loc_low += x->buffer_size_low;
        }

        x->buffer_low = (t_sample *)sysmem_newptr(x->buffer_size_low * sizeof(t_sample));
        
        if(x->buffer_low){
            for (int i=0; i < x->buffer_size_low; i++){
                x->buffer_low[i] = 0.0;
            }
        }
        
        if (x->buffer_low == NULL) {  //error checking to make sure buffer was made
            object_free(x);
            post("buffer not found");
            return NULL;
            }


//---------------------------------Flutter Buffer Initialization------------------------------------------------------------
    x->buffer_size_flutter = 44100*5;
    x->feedback_flutter = 0.4;
    x->write_loc_flutter = 0;
    x->read_loc_flutter = x->write_loc_flutter - 44100;
    
    if(x->read_loc_flutter < 0){
        x->read_loc_flutter += x->buffer_size_flutter;
    }
    
    x->buffer_flutter = (t_sample *)sysmem_newptr(x->buffer_size_flutter * sizeof(t_sample));
    
    if(x->buffer_flutter){
        for (int i=0; i < x->buffer_size_flutter; i++){
            x->buffer_flutter[i] = 0.0;
        }
    }
    
    if (x->buffer_flutter == NULL) {  //error checking to make sure buffer was made
        object_free(x);
        post("buffer not found");
        return NULL;
        }

//------------------------------Random Tones Initialization---------------------------------------------------------------
    x->random_number = rand()%(44100 + 1);
    x->random_number_temp = x->random_number;
    x->grain_size = 10000;
    
    x->random_number_two = rand()%(44100 + 1);
    x->random_number_temp_two = x->random_number_two;
    x->grain_size_two = 10000;
    return (x);
}



/*
 @brief: Free memory allocated by the Secret object when it is deleted in the patcher
 @params: Pointer to our struct object
 @return: [void]
 */
void secret_free(t_secret *x)
{
    dsp_free((t_pxobject *)x);
    if (x->buffer_normal) {
            sysmem_freeptr(x->buffer_normal);
        }
    if (x->buffer_low) {
            sysmem_freeptr(x->buffer_low);
        }
    if (x->buffer_high) {
            sysmem_freeptr(x->buffer_high);
        }
    if (x->buffer_flutter) {
            sysmem_freeptr(x->buffer_flutter);
        }
    post("object memory freed");
}

/*
 @brief:
 @params:
 @return:
 */
void secret_assist(t_secret *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET){
        switch (a){
            case 0:  sprintf(s,"(Signal) Signal In"); break;
            case 1:  sprintf(s,"(Int) Grain Size 1"); break;
            case 2:  sprintf(s,"(Int) Grain Size 2"); break;
        }
    }
    else {
        switch (a) {
            case 0: sprintf(s,"(Signal/Float) Normal Delay"); break;
            case 1: sprintf(s,"(Signal/Float) Flutter Dealy"); break;
            case 2: sprintf(s,"(Signal/Float) Fluttered Normal Delay"); break;
            case 3: sprintf(s,"(Signal/Float) Low Delay"); break;
            case 4: sprintf(s,"(Signal/Float) High Delay"); break;
            case 5: sprintf(s,"(Signal/Float) Random Tones 1"); break;
            case 6: sprintf(s,"(Signal/Float) Random Tones 2"); break;
        }
    }
    
}

/*
 @brief: Registers a function for the signal chain in Max. Mysterious function that allows for DSP to function when the EZDAC object is turned on
 @params: ...not sure what these all mean...
 @return: [void]
 */
void secret_dsp64(t_secret *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    object_method(dsp64, gensym("dsp_add64"), x, secret_perform64, 0, NULL);
}


/*
 @brief: This is the 64-bit perform method audio vectors. Where the signal is manipulated.
 @params: Pointer to struct, dsp info, inlet info, outlet info, sampleframes (64 bit vector here), object arguements
 @return: [void]
 */
void secret_perform64(t_secret *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{

    double *inL = ins[0];  // Setting signal in to inlet 1
    double *out_one = outs[0];  // Setting signal out to the 8 outlets
    double *out_two = outs[1];
    double *out_three = outs[2];
    double *out_four = outs[3];
    double *out_five = outs[4];
    double *out_six = outs[5];
    double *out_seven = outs[6];
    long n = sampleframes; // Setting sampleframes equal to variable because I'm not sure if sampleframes is a const...don't want to mess with that...creates serious artifacting.

    while(n--){
        
        *out_one = normal_delay(x, *inL, n) + *inL;//Normal delay and original signal in outlet 1
        *out_two = flutter_delay(x, *inL, n); // Flutter delay in outlet 2
        *out_three = flutter_delay(x, normal_delay(x, *inL, n), n); // Normal delay passed through flutter delay in outlet 3
        *out_four = low_pitch(x, *inL, n); // Low pitched delay in outlet 4
        *out_five = high_pitch(x, *inL, n); // High pitched delay in outlet 5
        *out_six = random_tones(x, *inL, n); // Random Tones granulation in outlet 6
        *out_seven = random_tones_offset(x, *inL, n); // Offset random tones granulation in 7
        
        inL++; // Need to constantly update sample location for both inlets/outlets or else the signal gets "stuck"
        out_one++;
        out_two++;
        out_three++;
        out_four++;
        out_five++;
        out_six++;
        out_seven++;
    }
    
}


/*
 @brief: Normal feedback delay function
 @params: Pointer to object, signal in, and sampleframes
 @return: Buffer location to be read
 */
double normal_delay (t_secret *x, double inL, long n)
{

    x->buffer_normal[x->write_loc] = inL + (x->buffer_normal[x->read_loc] * x->feedback); // Current buffer location is written with signal in and feedback of previous signals

        if(x->write_loc >= x->buffer_size){
            x->write_loc = 0; // Makes sure location is never out of bounds
        }
        else{
            (x->write_loc)++; // Current write-to-buffer location updated
        }

        if(x->read_loc >= x->buffer_size){
            x->read_loc = 0; // Makes sure location is never out of bounds
        }
        else{
            (x->read_loc)++; // Current write-to-buffer location updated
        }

    return (x->buffer_normal[x->read_loc]); //Current buffer-read-location is returned and outputted
}

/*
 @brief: A feedback delay that is pitched up an octave. Pitching works by reading only every other sample that is written...has unintended artifacting that can sound quite nice. Otherwise works the same as the normal delay.
 @params: Pointer to object, signal in, and sampleframes
 @return: Buffer location to be read
 */
double high_pitch (t_secret *x, double inL, long n)
{
    x->buffer_high[x->write_loc_high] = inL + (x->buffer_high[x->read_loc_high] * x->feedback_high); //

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
            (x->read_loc_high)+=2; // Note the read position updates by 2 spots
        }
    
//// Fixes decay into noise --> potential for granulation here with random function
//        if(x->read_loc_high == x->write_loc_high){
    //            x->read_loc_high = x->write_loc_high+1;
//        }

    return (x->buffer_high[x->read_loc_high]);
}





/*
 @brief: Identical to Normal Delay, but with a much shorter delay time. Good for passing signal from other delay functions into it for cool effects
 @params: Pointer to object, signal in, and sampleframes
 @return: Buffer location to be read
 */
double flutter_delay (t_secret *x, double inL, long n)
{
    x->buffer_flutter[x->write_loc_flutter] = inL + (x->buffer_flutter[x->read_loc_flutter] * x->feedback_flutter);

        if(x->write_loc_flutter >= x->buffer_size_flutter){
            x->write_loc_flutter = 0;
        }
        else{
            (x->write_loc_flutter)++;
        }
        if(x->read_loc_flutter >= x->buffer_size_flutter){
            x->read_loc_flutter = 0;
        }
        else{
            (x->read_loc_flutter)++;
        }

    return (x->buffer_flutter[x->read_loc_flutter]);
}





/*
 @brief: Boolean switch is used so that only every other sample is written to the buffer. A temp variable is used to duplicate the samples to reduce artifacting
 @params: Pointer to object, signal in, and sampleframes
 @return: Buffer location to be read
 */
double low_pitch(t_secret *x, double inL, long n)
{

double current_in = inL + (x->buffer_low[x->read_loc_low] * x->feedback_low);

    if(x->switch_low){
        //write input to the current write location
        x->buffer_low[(x->write_loc_low)] = current_in;

        //write input to to temp variable
        x->temp_low = current_in;

        if((x->write_loc_low) >= x->buffer_size_low){
            (x->write_loc_low) = 0;
        }
        else{
            (x->write_loc_low)++;
        }
    }
    else if (x->switch_low == false){
        //write temp variable to next write location
        x->buffer_low[(x->write_loc_low)] = x->temp_low;

        if((x->write_loc_low) >= x->buffer_size_low){
            (x->write_loc_low) = 0;
        }
        else{
            (x->write_loc_low)++;
        }

        x->buffer_low[(x->write_loc_low)] = x->temp_low;

        if((x->write_loc_low) >= x->buffer_size_low){
            (x->write_loc_low) = 0;
        }
        else{
            (x->write_loc_low)++;
        }
    }


    if(x->read_loc_low >= x->buffer_size_low){
        x->read_loc_low = 0;
    }
    else{
        (x->read_loc_low)++;
    }

    x->switch_low = !(x->switch_low); // Boolean switches



    return (x->buffer_low[x->read_loc_low]);
    }

/*
 @brief: Samples from a random place in normal buffer are played at a certain grain size
 @params: Pointer to object, signal in, and sampleframes
 @return: Buffer location to be read
 */
double random_tones(t_secret *x, double inL, long n)
{
    if (x->random_number >= (x->random_number_temp + x->grain_size) || x->random_number >= x->buffer_size){
        x->random_number = x->random_number_temp;
    }
    else{
        x->random_number++;
    }


    return (x->buffer_normal[x->random_number]);
}


/*
 @brief: Same as above but with a different random starting place
 @params: Pointer to object, signal in, and sampleframes
 @return: Buffer location to be read
 */
double random_tones_offset(t_secret *x, double inL, long n)
{
    if (x->random_number_two >= (x->random_number_temp_two + (x->grain_size_two)) || x->random_number_two >= x->buffer_size){
        x->random_number_two = x->random_number_temp_two;
    }
    else{
        x->random_number_two++;
    }


    return (x->buffer_normal[x->random_number_two]);
}



/*
 @brief: Input from second inlet (first Max inlet)
 */
void secret_in3 (t_secret *x, long q)
{
    x->grain_size = q;
}

/*
 @brief: Input from second inlet (first Max inlet)
 */
void secret_in2 (t_secret *x, long q)
{
    x->grain_size_two = q;
}
