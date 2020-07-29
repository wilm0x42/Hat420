#include <stdlib.h>
#include <string.h>

#include "ladspa.h"
#include "utils.h"

#define NOISE_AMPLITUDE 0
#define NOISE_SUSTAIN	1
#define NOISE_OUTPUT	2
#define AUDIO_INPUT		3

typedef struct
{
	LADSPA_Data* amplitudeValue;
	LADSPA_Data* sustain;
	LADSPA_Data* outputBuffer;
	LADSPA_Data* inputBuffer;
	
	float currentPeak;
} Hat420;

// Construct a new plugin instance
static LADSPA_Handle instantiateHat420(const LADSPA_Descriptor* Descriptor, unsigned long SampleRate)
{
	return malloc(sizeof(Hat420));
}

// Connect a port to a data location
static void connectPortToHat420(LADSPA_Handle Instance, unsigned long Port, LADSPA_Data* DataLocation)
{
	switch (Port)
	{
	case NOISE_AMPLITUDE:
		((Hat420*)Instance)->amplitudeValue = DataLocation;
		break;
	case NOISE_OUTPUT:
		((Hat420*)Instance)->outputBuffer = DataLocation;
		break;
	case AUDIO_INPUT:
		((Hat420*)Instance)->inputBuffer = DataLocation;
		break;
	case NOISE_SUSTAIN:
		((Hat420*)Instance)->sustain = DataLocation;
		break;
	}
	
	((Hat420*)Instance)->currentPeak = 0.0; // wait heck this should go in a different function shouldn't it
}

// Run a hat420 instance for a block of SampleCount samples
static void runHat420(LADSPA_Handle Instance, unsigned long SampleCount)
{
	Hat420* hat420 = (Hat420*)Instance;
	
	LADSPA_Data* output = hat420->outputBuffer;
	LADSPA_Data* input = hat420->inputBuffer;
	LADSPA_Data amplitude = *(hat420->amplitudeValue);
	LADSPA_Data sustain = *(hat420->sustain);
	float currentPeak = hat420->currentPeak;
	
	unsigned long sampleIndex;
	for (sampleIndex = 0; sampleIndex < SampleCount; sampleIndex++)
	{	
		float in = *(input++);
		
		currentPeak -= (currentPeak/sustain) * (currentPeak/sustain);
		
		if (in > currentPeak)
			currentPeak = in;
		
		*(output++) = ((((float)rand() / (float)RAND_MAX) * 2.0) - 1.0) * amplitude * currentPeak;
	}
}

static void cleanupHat420(LADSPA_Handle Instance)
{
	free(Instance);
}

static LADSPA_Descriptor* descriptor;

ON_LOAD_ROUTINE
{
	char** portNames;
	LADSPA_PortDescriptor* portDescriptors;
	LADSPA_PortRangeHint* portRangeHints;

	descriptor = (LADSPA_Descriptor*)malloc(sizeof(LADSPA_Descriptor));

	if (descriptor)
	{

		descriptor->UniqueID = 42069;
		descriptor->Label = strdup("hat420");
		descriptor->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
		descriptor->Name = strdup("HighHat420");
		descriptor->Maker = strdup("Walm");
		descriptor->Copyright = strdup("None");
		descriptor->PortCount = 4;
		
		portDescriptors = (LADSPA_PortDescriptor*)calloc(4, sizeof(LADSPA_PortDescriptor));
		descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)portDescriptors;
		
		portDescriptors[NOISE_AMPLITUDE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		portDescriptors[NOISE_SUSTAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
		portDescriptors[NOISE_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		portDescriptors[AUDIO_INPUT] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		
		portNames = (char**)calloc(4, sizeof(char*));
		descriptor->PortNames = (const char**)portNames;
		portNames[NOISE_AMPLITUDE] = strdup("Amplitude");
		portNames[NOISE_SUSTAIN] = strdup("Sustain");
		portNames[NOISE_OUTPUT] = strdup("Output");
		portNames[AUDIO_INPUT] = strdup("Input");
		
		portRangeHints = ((LADSPA_PortRangeHint*)calloc(4, sizeof(LADSPA_PortRangeHint)));
		descriptor->PortRangeHints = (const LADSPA_PortRangeHint*)portRangeHints;
		
		portRangeHints[NOISE_AMPLITUDE].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW
														| LADSPA_HINT_LOGARITHMIC
														| LADSPA_HINT_DEFAULT_1;
		portRangeHints[NOISE_AMPLITUDE].LowerBound = 0;
		
		portRangeHints[NOISE_SUSTAIN].HintDescriptor = LADSPA_HINT_BOUNDED_BELOW
														| LADSPA_HINT_BOUNDED_ABOVE
														| LADSPA_HINT_DEFAULT_MIDDLE;
		portRangeHints[NOISE_SUSTAIN].LowerBound = 1;
		portRangeHints[NOISE_SUSTAIN].UpperBound = 100;
		
		portRangeHints[NOISE_OUTPUT].HintDescriptor = 0;
		portRangeHints[AUDIO_INPUT].HintDescriptor = 0;
		
		descriptor->instantiate = instantiateHat420;
		descriptor->connect_port = connectPortToHat420;
		descriptor->activate = NULL;
		descriptor->run = runHat420;
		descriptor->run_adding = NULL;
		descriptor->deactivate = NULL;
		descriptor->cleanup = cleanupHat420;
	}
}

ON_UNLOAD_ROUTINE
{
	long index;
	if (descriptor)
	{
		free((char*)descriptor->Label);
		free((char*)descriptor->Name);
		free((char*)descriptor->Maker);
		free((char*)descriptor->Copyright);
		free((LADSPA_PortDescriptor*)descriptor->PortDescriptors);
		
		for (index = 0; index < descriptor->PortCount; index++)
			free((char*)descriptor->PortNames[index]);
		
		free((char**)descriptor->PortNames);
		free((LADSPA_PortRangeHint *)descriptor->PortRangeHints);
		free(descriptor);
	}
}

// Return a descriptor of the requested plugin type
const LADSPA_Descriptor* ladspa_descriptor(unsigned long Index)
{
	if (Index == 0)
		return descriptor;
	else
		return NULL;
}