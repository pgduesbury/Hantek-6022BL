# Hantek-6022BL
Oscilloscope software for up to 48Ms/s with delayed trigger, hold off and robust triggering

Based on Open6022-QT by G Carmix,  incorporating the driver and firmware uploader by Rodrigo Pedroso Mendes and the QCustomPlot widget by Emanuel Eichhammer.  Distributed under the GNU public License v 3.0.

This adaptation is designed to run on a battery powered atom notebook PC with a 1024*600 display under lubuntu 14.04.  It attempts to make the 6022 behave more like a conventional CRT oscilloscope, maintaining the highest practical sampling rate at all timebase settings and addressing the limitations of the hardware at 48Ms/s.


FEATURES

-  Aggressive search for trigger edge increases useful display refresh rate at maximum sampling rate.

-  Improved noise immunity on the trigger and more accurate trigger edge location by interpolation.

-  Five fold upsampling on fastest timebase settings stabilises trace close to the Nyquist limit.

-  Delayed trigger accepts both positive (typically up to 16ms) and negative offsets.

-  Variable hold-off of between 0 and 200ms.

-  Conventional add, invert and variable vertical sensitivity facilitate differential trace display.

-  Glitch mode shows pulses shorter than the displayed sampling interval at slower timebase settings.

The usual Auto, Normal and Single shot modes are supported, triggering on either a rising or falling edge.   There are no explicit measurement facilities or cursors although both the trigger delay and vertical offset controls have an associated numeric display which can be used instead in conjunction with the reticule.

At 48Ms/s the useful trace buffer length is only a little over 1000 samples and the trigger edge can occur anywhere within this.  To reduce flicker and provide a more useful and complete display, a composite of successive scans is presented, thereby filling in missing data further from the trigger edge.


INSTALLATION

The easiest way to build and run this program is to install QTCreator from the repository using Synaptic or apt-get.  This should bring in the mesa OpenGL utilities and other necessary libraries along with the rather nice IDE.  Build-essential is almost certainly required too.  Associate the .pro file with QTCreator and click on the file to launch the IDE.

libusb is needed and should already be present in most linux distributions but you may need to install the corresponding dev package from the repository.  Check the locations of the header and library files specified in the .pro file if during compilation an error is flagged in respect of lbusb.h.

The binary needs access to the Hantek 'scope USB device.  Running it as root should provide the necessary privileges and this may be acceptable for a quick test.  On Ubuntu, users are members of the "plugdev" group so a better solution is to copy the .rules file provided to /etc/udev/rules.d/. and restart udev either explicitly or by re-booting.  There are two entries for the Hantek 'scope in the file as the USB ID changes following firmware upload but make sure that the USB ident button on the 'scope is pressed in before connecting it to the PC and do this before starting the program otherwise the firmware will not be uploaded when the program starts.

If high bandwidth devices such as wireless adapters share the USB bus, these should be disabled as they will disrupt waveform capture.


OPERATION

The program starts in the idle state.  Click "ARM" to initiate waveform capture.  The two traces are initially superimposed although there may be a small offset with the default calibration parameters.  Run the calibration routine with both probes grounded to fix this.  It only takes a few seconds and generates a text file .scope in the home directory.  This file contains offset values for both channels and all input ranges, followed by a single vertical scaling factor which will initially be set to 1.0.  At present, the only way to change this scale factor is to edit the file.  I use a value of 1.06 which is based on measurement of a 1.5V cell using both the 'scope and a digital multi-meter, saving the trace to a file and looking at the values recorded.

The calibrator output is a 2.0V p-p square wave at 1KHz and provides a useful test signal for almost all timebase settings.  The multi-turn delayed trigger control can be used to view the waveform a short time before the trigger event by setting a negative delay.  The extent of the available pre-trigger waveform depends on the rate at which trigger events occur.


KNOWN ISSUES

-  Pre-trigger display is not well behaved when viewing high frequency waveforms on upsampled ranges with a negative trigger delay.

-  No mechanism to set calibrated vertical scale factor within program.


Paul Duesbury

