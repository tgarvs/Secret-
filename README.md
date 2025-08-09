# Secret
This is a Max MSP external patch used as an exhibition art piece. The basic idea is that both a microphone and a GameTrak controller are connected to a Max MSP patch containing this external. The participant speaks a secret into the microphone, which is then played back to them in a variety of manners, all of which are handled through the external. Naturally, the participant will feel uncomfortable and move unconsciously. They should be holding the GameTrack controllers, which feed data into the patch. This data corresponds to the output level of each output from the external.

Note that this external can be used as a mutlivariable delay without using this specialty controller (see image). 

<img width="247" height="204" alt="image" src="https://github.com/user-attachments/assets/72c2344e-22fd-4107-a12d-52ee3c08a3ca" />


# Overall Design
The external is a series of concurrent delays run by circular buffers. Each delay has a different playback time and characteristic, including on that slows down and drops the input's pitch and another that speeds it up. The last two outputs also work as psuedo-granulations, whose parameters are also controlled by the position of the Gametrak controllers.

# TODO
- Make sure the object retains interobject connections when loaded
- Smooth artifacting
