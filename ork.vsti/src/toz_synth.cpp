///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

#include "toz_synth.h"

///////////////////////////////////////////////////////////////////////////////

AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	return new TozSynth (audioMaster);
}

///////////////////////////////////////////////////////////////////////////////

TozSynthProgram::TozSynthProgram()
	: mVolume(1.0f)
{
	vst_strncpy (mName, "Yo", kVstMaxProgNameLen);
}
TozSynthProgram::~TozSynthProgram() {}

///////////////////////////////////////////////////////////////////////////////

TozSynth::TozSynth (audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, kNumPrograms, kNumParams)
	, mPhaseAccumulator(0.0f)
{
	// initialize programs
	programs = new TozSynthProgram[kNumPrograms];
	for (VstInt32 i = 0; i < 16; i++)
		channelPrograms[i] = i;

	if (programs)
		setProgram (0);
	
	if (audioMaster)
	{
		setNumInputs (0);				// no inputs
		setNumOutputs (kNumOutputs);	// 2 outputs, 1 for each oscillator
		canProcessReplacing ();
		isSynth ();
		setUniqueID ('TOZa');			// <<<! *must* change this!!!!
	}

	initProcess ();
	suspend ();
}

///////////////////////////////////////////////////////////////////////////////

TozSynth::~TozSynth ()
{
	if (programs)
		delete[] programs;
}

///////////////////////////////////////////////////////////////////////////////
void TozSynth::setProgram (VstInt32 program)
{
	if (program < 0 || program >= kNumPrograms)
		return;
	
	TozSynthProgram *ap = &programs[program];
	curProgram = program;
		
	mVolume    = ap->mVolume;
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::setProgramName (char* name)
{
	vst_strncpy (programs[curProgram].mName, name, kVstMaxProgNameLen);
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::getProgramName (char* name)
{
	vst_strncpy (name, programs[curProgram].mName, kVstMaxProgNameLen);
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::getParameterLabel (VstInt32 index, char* label)
{
	switch (index)
	{
		case kVolume:
			vst_strncpy (label, "dB", kVstMaxParamStrLen);
			break;
	}
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::getParameterDisplay (VstInt32 index, char* text)
{
	text[0] = 0;
	switch (index)
	{
		case kVolume:
			dB2string (mVolume, text, kVstMaxParamStrLen);
			break;
	}
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::getParameterName (VstInt32 index, char* label)
{
	switch (index)
	{
		case kVolume:
			vst_strncpy (label, "Volume", kVstMaxParamStrLen);
			break;
	}
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::setParameter (VstInt32 index, float value)
{
	auto ap = & programs[curProgram];

	switch (index)
	{
		case kVolume:
			mVolume	= ap->mVolume = value;
			break;
	}
}
///////////////////////////////////////////////////////////////////////////////
float TozSynth::getParameter (VstInt32 index)
{
	float value = 0;
	switch (index)
	{
		case kVolume:
			value = mVolume;
			break;
	}
	return value;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{
	if (index < kNumOutputs)
	{
		vst_strncpy (properties->label, "Vstx ", 63);
		char temp[11] = {0};
		int2string (index + 1, temp, 10);
		vst_strncat (properties->label, temp, 63);

		properties->flags = kVstPinIsActive;
		if (index < 2)
			properties->flags |= kVstPinIsStereo;	// make channel 1+2 stereo
		return true;
	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
	if (index < kNumPrograms)
	{
		vst_strncpy (text, programs[index].mName, kVstMaxProgNameLen);
		return true;
	}
	return false;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::getEffectName (char* name)
{
	vst_strncpy (name, "TozSynth", kVstMaxEffectNameLen);
	return true;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::getVendorString (char* text)
{
	vst_strncpy (text, "tweakoz", kVstMaxVendorStrLen);
	return true;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::getProductString (char* text)
{
	vst_strncpy (text, "TozSynth", kVstMaxProductStrLen);
	return true;
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::getVendorVersion ()
{ 
	return 1000; 
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::canDo (char* text)
{
	if (!strcmp (text, "receiveVstEvents"))
		return 1;
	if (!strcmp (text, "receiveVstMidiEvent"))
		return 1;
	if (!strcmp (text, "midiProgramNames"))
		return 1;
	return -1;	// explicitly can't do; 0 => don't know
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::getNumMidiInputChannels ()
{
	return 1; // we are monophonic
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::getNumMidiOutputChannels ()
{
	return 0; // no MIDI output back to Host app
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::getMidiProgramName (VstInt32 channel, MidiProgramName* mpn)
{
	VstInt32 prg = mpn->thisProgramIndex;
	if (prg < 0 || prg >= 128)
		return 0;
	fillProgram (channel, prg, mpn);
	if (channel == 9)
		return 1;
	return 128L;
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::getCurrentMidiProgram (VstInt32 channel, MidiProgramName* mpn)
{
	if (channel < 0 || channel >= 16 || !mpn)
		return -1;
	VstInt32 prg = channelPrograms[channel];
	mpn->thisProgramIndex = prg;
	fillProgram (channel, prg, mpn);
	return prg;
}
///////////////////////////////////////////////////////////////////////////////
void TozSynth::fillProgram (VstInt32 channel, VstInt32 prg, MidiProgramName* mpn)
{
	mpn->midiBankMsb =
	mpn->midiBankLsb = -1;
	mpn->reserved = 0;
	mpn->flags = 0;
	mpn->midiProgram = 0;
	mpn->parentCategoryIndex = 0;
	vst_strncpy (mpn->name, "Yo", 63);
}
///////////////////////////////////////////////////////////////////////////////
VstInt32 TozSynth::getMidiProgramCategory (VstInt32 channel, MidiProgramCategory* cat)
{
	cat->parentCategoryIndex = -1;	// -1:no parent category
	cat->flags = 0;	// reserved, none defined yet, zero.
	VstInt32 category = cat->thisCategoryIndex;
	vst_strncpy (cat->name, "Toz", 63);
	return 1;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::hasMidiProgramsChanged (VstInt32 channel)
{
	return false;
}
///////////////////////////////////////////////////////////////////////////////
bool TozSynth::getMidiKeyName (VstInt32 channel, MidiKeyName* key)
{
	key->keyName[0] = 0;
	key->reserved = 0; // zero
	key->flags = 0;	// reserved, none defined yet, zero.
	return false;
}
