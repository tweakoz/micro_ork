///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

#include "toz_synth.h"
#include <math.h>

///////////////////////////////////////////////////////////////////////////////

static const int kNumMidiNotes = 128;
static const float kInv127 = (1.0f / 127.0f);
static float freqtab[kNumMidiNotes];

///////////////////////////////////////////////////////////////////////////////

void TozSynth::setSampleRate (float sampleRate)
{
	AudioEffectX::setSampleRate (sampleRate);
	mSampleRate = sampleRate;
	mPhaseIncCoef = (2.0f*3.1415926 / mSampleRate);
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::setBlockSize (VstInt32 blockSize)
{
	AudioEffectX::setBlockSize (blockSize);
	// you may need to have to do something here...
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::initProcess ()
{
	mPhaseAccumulator = 0.0f;
	noteIsOn = false;
	currentDelta = currentNote = currentDelta = 0;
	VstInt32 i;

	// make frequency (Hz) table
	double k = 1.059463094359;	// 12th root of 2
	double a = 6.875;	// a
	a *= k;	// b
	a *= k;	// bb
	a *= k;	// c, frequency of midi note 0
	for (i = 0; i < kNumMidiNotes; i++)	// 128 midi notes
	{
		freqtab[i] = (float)a;
		a *= k;
	}
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	float* out1 = outputs[0];
	float* out2 = outputs[1];

	if (noteIsOn)
	{
		float f = freqtab[currentNote & 0x7f];
		float vol = mVolume * currentVelocity * kInv127;
		
		if (currentDelta > 0)
		{
			if (currentDelta >= sampleFrames)	// future
			{
				currentDelta -= sampleFrames;
				return;
			}
			memset (out1, 0, currentDelta * sizeof (float));
			memset (out2, 0, currentDelta * sizeof (float));
			out1 += currentDelta;
			out2 += currentDelta;
			sampleFrames -= currentDelta;
			currentDelta = 0;
		}

		// loop
		while (--sampleFrames >= 0)
		{
			float fs = sinf(mPhaseAccumulator);
			(*out1++) = fs * vol;
			(*out2++) = fs * vol;
			mPhaseAccumulator += (f*mPhaseIncCoef);
		}
	}						
	else
	{
		memset (out1, 0, sampleFrames * sizeof (float));
		memset (out2, 0, sampleFrames * sizeof (float));
	}
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::processEvents (VstEvents* ev)
{
	for (VstInt32 i = 0; i < ev->numEvents; i++)
	{
		if ((ev->events[i])->type != kVstMidiType)
			continue;

		VstMidiEvent* event = (VstMidiEvent*)ev->events[i];
		char* midiData = event->midiData;
		VstInt32 status = midiData[0] & 0xf0;	// ignoring channel
		if (status == 0x90 || status == 0x80)	// we only look at notes
		{
			VstInt32 note = midiData[1] & 0x7f;
			VstInt32 velocity = midiData[2] & 0x7f;
			if (status == 0x80)
				velocity = 0;	// note off by velocity 0
			if (!velocity && (note == currentNote))
				noteOff ();
			else
				noteOn (note, velocity, event->deltaFrames);
		}
		else if (status == 0xb0)
		{
			if (midiData[1] == 0x7e || midiData[1] == 0x7b)	// all notes off
				noteOff ();
		}
		event++;
	}
	return 1;
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::noteOn (VstInt32 note, VstInt32 velocity, VstInt32 delta)
{
	currentNote = note;
	currentVelocity = velocity;
	currentDelta = delta;
	noteIsOn = true;
	mPhaseAccumulator = 0.0f;
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::noteOff ()
{
	noteIsOn = false;
}
