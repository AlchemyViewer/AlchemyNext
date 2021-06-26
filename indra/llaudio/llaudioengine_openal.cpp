/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "lldir.h"

#include "llaudioengine_openal.h"
#include "lllistener_openal.h"


const float LLAudioEngine_OpenAL::WIND_BUFFER_SIZE_SEC = 0.05f;

LLAudioEngine_OpenAL::LLAudioEngine_OpenAL()
	:
	mWindGen(NULL),
	mWindBuf(NULL),
	mWindBufFreq(0),
	mWindBufSamples(0),
	mWindBufBytes(0),
	mWindSource(AL_NONE),
	mNumEmptyWindALBuffers(MAX_NUM_WIND_BUFFERS)
{
}

// virtual
bool LLAudioEngine_OpenAL::init(const S32 num_channels, void* userdata, const std::string &app_title)
{
	mWindGen = NULL;
	LLAudioEngine::init(num_channels, userdata, app_title);

	if(!alutInit(NULL, NULL))
	{
		ALOG_AUDIO_WARN("LLAudioEngine_OpenAL::init() ALUT initialization failed: {}", alutGetErrorString (alutGetError()));
		return false;
	}

	ALOG_AUDIO_INFO("LLAudioEngine_OpenAL::init() OpenAL successfully initialized");

	ALOG_AUDIO_INFO("OpenAL version: {}", ll_safe_string(alGetString(AL_VERSION)));
	ALOG_AUDIO_INFO("OpenAL vendor: {}", ll_safe_string(alGetString(AL_VENDOR)));
	ALOG_AUDIO_INFO("OpenAL renderer: {}", ll_safe_string(alGetString(AL_RENDERER)));

	ALint major = alutGetMajorVersion ();
	ALint minor = alutGetMinorVersion ();
	ALOG_AUDIO_INFO("ALUT version: {}.{}", major, minor);

	ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());

	alcGetIntegerv(device, ALC_MAJOR_VERSION, 1, &major);
	alcGetIntegerv(device, ALC_MINOR_VERSION, 1, &minor);
	ALOG_AUDIO_INFO("ALC version: {}.{}", major, minor);

	ALOG_AUDIO_INFO("ALC default device: {}", ll_safe_string(alcGetString(device, ALC_DEFAULT_DEVICE_SPECIFIER)));

	return true;
}

// virtual
std::string LLAudioEngine_OpenAL::getDriverName(bool verbose)
{
	ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());
	std::ostringstream version;

	version <<
		"OpenAL";

	if (verbose)
	{
		version <<
			", version " <<
			ll_safe_string(alGetString(AL_VERSION)) <<
			" / " <<
			ll_safe_string(alGetString(AL_VENDOR)) <<
			" / " <<
			ll_safe_string(alGetString(AL_RENDERER));
		
		if (device)
			version <<
				": " <<
				ll_safe_string(alcGetString(device,
				    ALC_DEFAULT_DEVICE_SPECIFIER));
	}

	return version.str();
}

// virtual
void LLAudioEngine_OpenAL::allocateListener()
{
	mListenerp = (LLListener *) new LLListener_OpenAL();
	if(!mListenerp)
	{
		ALOG_AUDIO_WARN("Listener creation failed");
	}
}

// virtual
void LLAudioEngine_OpenAL::shutdown()
{
	ALOG_AUDIO_INFO("About to LLAudioEngine::shutdown()");
	LLAudioEngine::shutdown();

	ALOG_AUDIO_INFO("About to alutExit()");
	if(!alutExit())
	{
		ALOG_AUDIO_WARN("ALUT shutdown failed: {}", alutGetErrorString(alutGetError ()));
	}

	ALOG_AUDIO_INFO("OpenAL successfully shut down");

	delete mListenerp;
	mListenerp = NULL;
}

LLAudioBuffer *LLAudioEngine_OpenAL::createBuffer()
{
	return new LLAudioBufferOpenAL();
}

LLAudioChannel *LLAudioEngine_OpenAL::createChannel()
{
	return new LLAudioChannelOpenAL();
}

void LLAudioEngine_OpenAL::setInternalGain(F32 gain)
{
	//ALOG_AUDIO_INFO("Gain: {}", gain);
	alListenerf(AL_GAIN, gain);
}

LLAudioChannelOpenAL::LLAudioChannelOpenAL()
	:
	mALSource(AL_NONE),
	mLastSamplePos(0)
{
	alGenSources(1, &mALSource);
}

LLAudioChannelOpenAL::~LLAudioChannelOpenAL()
{
	cleanup();
	alDeleteSources(1, &mALSource);
}

void LLAudioChannelOpenAL::cleanup()
{
	alSourceStop(mALSource);
	alSourcei(mALSource, AL_BUFFER, AL_NONE);

	mCurrentBufferp = NULL;
}

void LLAudioChannelOpenAL::play()
{
	if (mALSource == AL_NONE)
	{
		ALOG_AUDIO_WARN("Playing without a mALSource, aborting");
		return;
	}

	if(!isPlaying())
	{
		alSourcePlay(mALSource);
		getSource()->setPlayedOnce(true);
	}
}

void LLAudioChannelOpenAL::playSynced(LLAudioChannel *channelp)
{
	if (channelp)
	{
		LLAudioChannelOpenAL *masterchannelp =
			(LLAudioChannelOpenAL*)channelp;
		if (mALSource != AL_NONE &&
		    masterchannelp->mALSource != AL_NONE)
		{
			// we have channels allocated to master and slave
			ALfloat master_offset;
			alGetSourcef(masterchannelp->mALSource, AL_SEC_OFFSET,
				     &master_offset);

			ALOG_AUDIO_INFO("Syncing with master at {} sec", master_offset);
			// *TODO: detect when this fails, maybe use AL_SAMPLE_
			alSourcef(mALSource, AL_SEC_OFFSET, master_offset);
		}
	}
	play();
}

bool LLAudioChannelOpenAL::isPlaying()
{
	if (mALSource != AL_NONE)
	{
		ALint state;
		alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
		if(state == AL_PLAYING)
		{
			return true;
		}
	}
		
	return false;
}

bool LLAudioChannelOpenAL::updateBuffer()
{
    if (!mCurrentSourcep)
    {
        // This channel isn't associated with any source, nothing
        // to be updated
        return false;
    }

	if (LLAudioChannel::updateBuffer())
	{
		// Base class update returned true, which means that we need to actually
		// set up the source for a different buffer.
		LLAudioBufferOpenAL *bufferp = (LLAudioBufferOpenAL *)mCurrentSourcep->getCurrentBuffer();
		ALuint buffer = bufferp->getBuffer();
		alSourcei(mALSource, AL_BUFFER, buffer);
		mLastSamplePos = 0;
	}

	if (mCurrentSourcep)
	{
		alSourcef(mALSource, AL_GAIN,
			  mCurrentSourcep->getGain() * getSecondaryGain());
		alSourcei(mALSource, AL_LOOPING,
			  mCurrentSourcep->isLoop() ? AL_TRUE : AL_FALSE);
		alSourcef(mALSource, AL_ROLLOFF_FACTOR,
			  gAudiop->mListenerp->getRolloffFactor());
	}

	return true;
}


void LLAudioChannelOpenAL::updateLoop()
{
	if (mALSource == AL_NONE)
	{
		return;
	}

	// Hack:  We keep track of whether we looped or not by seeing when the
	// sample position looks like it's going backwards.  Not reliable; may
	// yield false negatives.
	//
	ALint cur_pos;
	alGetSourcei(mALSource, AL_SAMPLE_OFFSET, &cur_pos);
	if (cur_pos < mLastSamplePos)
	{
		mLoopedThisFrame = true;
	}
	mLastSamplePos = cur_pos;
}


void LLAudioChannelOpenAL::update3DPosition()
{
	if(!mCurrentSourcep)
	{
		return;
	}
	if (mCurrentSourcep->isAmbient())
	{
		alSource3f(mALSource, AL_POSITION, 0.0, 0.0, 0.0);
		alSource3f(mALSource, AL_VELOCITY, 0.0, 0.0, 0.0);
		alSourcei (mALSource, AL_SOURCE_RELATIVE, AL_TRUE);
	} else {
		LLVector3 float_pos;
		float_pos.setVec(mCurrentSourcep->getPositionGlobal());
		alSourcefv(mALSource, AL_POSITION, float_pos.mV);
		alSourcefv(mALSource, AL_VELOCITY, mCurrentSourcep->getVelocity().mV);
		alSourcei (mALSource, AL_SOURCE_RELATIVE, AL_FALSE);
	}

	alSourcef(mALSource, AL_GAIN, mCurrentSourcep->getGain() * getSecondaryGain());
}

LLAudioBufferOpenAL::LLAudioBufferOpenAL()
{
	mALBuffer = AL_NONE;
}

LLAudioBufferOpenAL::~LLAudioBufferOpenAL()
{
	cleanup();
}

void LLAudioBufferOpenAL::cleanup()
{
	if(mALBuffer != AL_NONE)
	{
		alGetError(); // clear error
		alDeleteBuffers(1, &mALBuffer);

		ALenum error = alutGetError();
		if(ALC_NO_ERROR != error)
		{
			ALOG_AUDIO_WARN("Error: {} when cleaning up a buffer", alutGetErrorString(error));
		}
		mALBuffer = AL_NONE;
	}
}

bool LLAudioBufferOpenAL::loadWAV(const std::string& filename)
{
	cleanup();
	mALBuffer = alutCreateBufferFromFile(filename.c_str());
	if(mALBuffer == AL_NONE)
	{
		ALenum error = alutGetError(); 
		if (LLFile::isfile(filename))
		{
			ALOG_AUDIO_WARN("LLAudioBufferOpenAL::loadWAV() Error loading {} {}", filename, alutGetErrorString(error));
		}
		else
		{
			// It's common for the file to not actually exist.
			ALOG_AUDIO_DEBUG("LLAudioBufferOpenAL::loadWAV() Error loading {} {}", filename, alutGetErrorString(error));
		}
		return false;
	}

	return true;
}

U32 LLAudioBufferOpenAL::getLength()
{
	if(mALBuffer == AL_NONE)
	{
		return 0;
	}
	ALint length;
	alGetBufferi(mALBuffer, AL_SIZE, &length);
	return length / 2; // convert size in bytes to size in (16-bit) samples
}

// ------------

bool LLAudioEngine_OpenAL::initWind()
{
	ALenum error;
	ALOG_AUDIO_INFO("wind init start");

	mNumEmptyWindALBuffers = MAX_NUM_WIND_BUFFERS;

	alGetError(); /* clear error */
	
	alGenSources(1,&mWindSource);
	
	if((error=alGetError()) != AL_NO_ERROR)
	{
		ALOG_AUDIO_WARN("Error creating wind sources: {}", error);
	}

	mWindGen = new LLWindGen<WIND_SAMPLE_T>;

	mWindBufFreq = mWindGen->getInputSamplingRate();
	mWindBufSamples = llceil(mWindBufFreq * WIND_BUFFER_SIZE_SEC);
	mWindBufBytes = mWindBufSamples * 2 /*stereo*/ * sizeof(WIND_SAMPLE_T);

	mWindBuf = new WIND_SAMPLE_T [mWindBufSamples * 2 /*stereo*/];

	if(mWindBuf==NULL)
	{
		ALOG_AUDIO_CRITICAL("Error creating wind memory buffer");
		return false;
	}

	ALOG_AUDIO_INFO("wind init done");

	return true;
}

void LLAudioEngine_OpenAL::cleanupWind()
{
	ALOG_AUDIO_INFO("LLAudioEngine_OpenAL::cleanupWind()");

	if (mWindSource != AL_NONE)
	{
		// detach and delete all outstanding buffers on the wind source
		alSourceStop(mWindSource);
		ALint processed;
		alGetSourcei(mWindSource, AL_BUFFERS_PROCESSED, &processed);
		while (processed--)
		{
			ALuint buffer = AL_NONE;
			alSourceUnqueueBuffers(mWindSource, 1, &buffer);
			alDeleteBuffers(1, &buffer);
		}

		// delete the wind source itself
		alDeleteSources(1, &mWindSource);

		mWindSource = AL_NONE;
	}
	
	delete[] mWindBuf;
	mWindBuf = NULL;

	delete mWindGen;
	mWindGen = NULL;
}

void LLAudioEngine_OpenAL::updateWind(LLVector3 wind_vec, F32 camera_altitude)
{
	LLVector3 wind_pos;
	F64 pitch;
	F64 center_freq;
	ALenum error;
	ALuint *buffers = NULL;
	
	if (!mEnableWind)
		return;
	
	if(!mWindBuf)
		return;
	
	if (mWindUpdateTimer.checkExpirationAndReset(LL_WIND_UPDATE_INTERVAL))
	{
		
		// wind comes in as Linden coordinate (+X = forward, +Y = left, +Z = up)
		// need to convert this to the conventional orientation DS3D and OpenAL use
		// where +X = right, +Y = up, +Z = backwards
		wind_vec.setVec(-wind_vec.mV[1], wind_vec.mV[2], -wind_vec.mV[0]);
		
		pitch = 1.0 + mapWindVecToPitch(wind_vec);
		center_freq = 80.0 * pow(pitch,2.5*(mapWindVecToGain(wind_vec)+1.0));
		
		mWindGen->mTargetFreq = (F32)center_freq;
		mWindGen->mTargetGain = (F32)mapWindVecToGain(wind_vec) * mMaxWindGain;
		mWindGen->mTargetPanGainR = (F32)mapWindVecToPan(wind_vec);
		
		alSourcei(mWindSource, AL_LOOPING, AL_FALSE);
		alSource3f(mWindSource, AL_POSITION, 0.0, 0.0, 0.0);
		alSource3f(mWindSource, AL_VELOCITY, 0.0, 0.0, 0.0);
		alSourcef(mWindSource, AL_ROLLOFF_FACTOR, 0.0);
		alSourcei(mWindSource, AL_SOURCE_RELATIVE, AL_TRUE);
	}

	// ok lets make a wind buffer now

	ALint processed, queued, unprocessed;
	alGetSourcei(mWindSource, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei(mWindSource, AL_BUFFERS_QUEUED, &queued);
	unprocessed = queued - processed;

	// ensure that there are always at least 3x as many filled buffers
	// queued as we managed to empty since last time.
	mNumEmptyWindALBuffers = llmin(mNumEmptyWindALBuffers + processed * 3 - unprocessed, MAX_NUM_WIND_BUFFERS-unprocessed);
	mNumEmptyWindALBuffers = llmax(mNumEmptyWindALBuffers, 0);

	//ALOG_AUDIO_INFO("mNumEmptyWindALBuffers: {} ({}:{})", mNumEmptyWindALBuffers, unprocessed, processed);

	//delete the old wind buffers
	buffers = new ALuint[processed];
	alGetError(); /* clear error */
	alSourceUnqueueBuffers(mWindSource, processed, &buffers[0]);
	error = alGetError();
	if(error != AL_NO_ERROR)
	{
		ALOG_AUDIO_WARN("LLAudioEngine_OpenAL::updateWind() error swapping (unqueuing) buffers");
	}
	else
	{
		alDeleteBuffers(processed, &buffers[0]);
	}
	// We dont need to keep track of the buffers' id now.
	delete[] buffers;
	buffers = NULL;

	//create the buffers for the empty wind buffers
	unprocessed += mNumEmptyWindALBuffers;
	buffers = new ALuint[mNumEmptyWindALBuffers];
	alGetError(); /* clear error */
	alGenBuffers(mNumEmptyWindALBuffers,&buffers[0]);
	if((error=alGetError()) != AL_NO_ERROR)
	{
		ALOG_AUDIO_WARN("LLAudioEngine_OpenAL::updateWind() Error creating wind buffer: {}", error);
		//break;
	}

	//fill the buffers with generated wind.
	int errors = 0;
	for(int i = 0; i < mNumEmptyWindALBuffers; i++)
	{
		alBufferData(buffers[i],
					AL_FORMAT_STEREO16,
					mWindGen->windGenerate(mWindBuf,
							mWindBufSamples),
					mWindBufBytes,
					mWindBufFreq);
		error = alGetError();
		if(error != AL_NO_ERROR)
		{
			ALOG_AUDIO_WARN("LLAudioEngine_OpenAL::updateWind() error swapping (bufferdata) buffers");
			errors++;
		}
	}

	//queue the buffers
	alSourceQueueBuffers(mWindSource, mNumEmptyWindALBuffers, &buffers[0]);
	error = alGetError();
	if(error != AL_NO_ERROR)
	{
		ALOG_AUDIO_WARN("LLAudioEngine_OpenAL::updateWind() error swapping (queuing) buffers");
	}

	mNumEmptyWindALBuffers = errors;
	// We dont need to keep track of the buffers' id now.
	delete[] buffers;
	buffers = NULL;


	//restart playing if not playing
	ALint playing;
	alGetSourcei(mWindSource, AL_SOURCE_STATE, &playing);
	if(playing != AL_PLAYING)
	{
		alSourcePlay(mWindSource);

		ALOG_AUDIO_DEBUG("Wind had stopped - probably ran out of buffers - restarting: {} now queued.", (unprocessed + mNumEmptyWindALBuffers));
	}
}

